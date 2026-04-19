#include "libtusk.h"
#include <tsk/libtsk.h>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>

namespace
{
    struct FragmentRange
    {
        uint64_t start_offset = 0;
        uint64_t end_offset = 0;
    };

    struct FileFragmentReport
    {
        std::string filename;
        std::string type;
        bool is_fragmented = false;
        bool is_deleted = false;
        uint64_t size = 0;
        std::vector<FragmentRange> fragments;
    };

    struct PartitionReport
    {
        std::string name;
        std::string desc;
        std::string volume_label;
        uint64_t start_offset = 0;
        uint64_t end_offset = 0;
        bool has_filesystem = false;
        std::string fs_type_name;
        uint32_t fs_block_size = 0;
        uint64_t fs_offset_in_image = 0;
        std::vector<FileFragmentReport> files;
    };

    bool is_dot_entry(const char *name)
    {
        return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
    }

    std::string join_path(const std::string &base, const char *name)
    {
        return base + "/" + name;
    }

    std::vector<FragmentRange> collect_file_fragments(TSK_FS_FILE *file, const TSK_FS_INFO *fs)
    {
        std::vector<FragmentRange> fragments;

        if (!file || !file->meta || !fs)
        {
            return fragments;
        }

        const TSK_FS_ATTR *attr = tsk_fs_file_attr_get(file);
        if (!attr)
        {
            return fragments;
        }

        if ((attr->flags & TSK_FS_ATTR_NONRES) == 0)
        {
            return fragments;
        }

        uint64_t remaining = static_cast<uint64_t>(attr->size);
        uint64_t skip = static_cast<uint64_t>(attr->nrd.skiplen);

        for (const TSK_FS_ATTR_RUN *run = attr->nrd.run; run != nullptr && remaining > 0; run = run->next)
        {
            if (run->len <= 0)
            {
                continue;
            }

            if (run->flags & (TSK_FS_ATTR_RUN_FLAG_SPARSE | TSK_FS_ATTR_RUN_FLAG_FILLER))
            {
                continue;
            }

            const uint64_t run_len_bytes = static_cast<uint64_t>(run->len) * fs->block_size;
            if (run_len_bytes == 0)
            {
                continue;
            }

            if (skip >= run_len_bytes)
            {
                skip -= run_len_bytes;
                continue;
            }

            const uint64_t fs_relative_start = static_cast<uint64_t>(run->addr) * fs->block_size + skip;
            const uint64_t available_in_run = run_len_bytes - skip;
            const uint64_t to_use = std::min(available_in_run, remaining);
            skip = 0;

            if (to_use == 0)
            {
                continue;
            }

            const uint64_t image_start = static_cast<uint64_t>(fs->offset) + fs_relative_start;
            fragments.push_back({image_start, image_start + to_use - 1});
            remaining -= to_use;
        }

        return fragments;
    }

    FileFragmentReport build_fragment_report(const std::string &full_path, TSK_FS_FILE *file, const TSK_FS_INFO *fs)
    {
        FileFragmentReport report;
        report.filename = full_path;

        if (!file || !file->meta)
        {
            return report;
        }

        if (file->meta->type == TSK_FS_META_TYPE_DIR)
        {
            report.type = "directory";
        }
        else if (file->meta->type == TSK_FS_META_TYPE_REG)
        {
            report.type = "file";
        }
        else
        {
            report.type = "other";
        }

        report.size = static_cast<uint64_t>(file->meta->size);
        report.is_deleted = (file->meta->flags & TSK_FS_META_FLAG_UNALLOC) != 0;
        report.fragments = collect_file_fragments(file, fs);

        report.is_fragmented = report.fragments.size() > 1;
        return report;
    }

    std::string escape_json_string(const std::string &value)
    {
        std::string escaped;
        escaped.reserve(value.size() + 8);

        for (char c : value)
        {
            switch (c)
            {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += c;
                break;
            }
        }

        return escaped;
    }

    std::string get_fs_type_name(TSK_FS_TYPE_ENUM ftype)
    {
        switch (ftype)
        {
        case TSK_FS_TYPE_NTFS:
            return "NTFS";
        case TSK_FS_TYPE_FAT12:
            return "FAT12";
        case TSK_FS_TYPE_FAT16:
            return "FAT16";
        case TSK_FS_TYPE_FAT32:
            return "FAT32";
        case TSK_FS_TYPE_EXFAT:
            return "exFAT";
        case TSK_FS_TYPE_FFS1:
            return "UFS1";
        case TSK_FS_TYPE_FFS1B:
            return "UFS1b";
        case TSK_FS_TYPE_FFS2:
            return "UFS2";
        case TSK_FS_TYPE_EXT2:
            return "ext2";
        case TSK_FS_TYPE_EXT3:
            return "ext3";
        case TSK_FS_TYPE_EXT4:
            return "ext4";
        case TSK_FS_TYPE_SWAP:
            return "SWAP";
        case TSK_FS_TYPE_RAW:
            return "RAW";
        case TSK_FS_TYPE_ISO9660:
            return "ISO9660";
        case TSK_FS_TYPE_HFS:
            return "HFS+";
        case TSK_FS_TYPE_HFS_LEGACY:
            return "HFS";
        case TSK_FS_TYPE_APFS:
            return "APFS";
        case TSK_FS_TYPE_YAFFS2:
            return "YAFFS2";
        default:
            return "UNKNOWN";
        }
    }

    std::string utf16le_to_utf8(const uint8_t *buf, size_t byte_len)
    {
        std::string out;
        for (size_t i = 0; i + 1 < byte_len; i += 2)
        {
            uint32_t ch = static_cast<uint32_t>(buf[i]) | (static_cast<uint32_t>(buf[i + 1]) << 8);
            if (ch == 0)
                break;
            if (ch < 0x80)
            {
                out += static_cast<char>(ch);
            }
            else if (ch < 0x800)
            {
                out += static_cast<char>(0xC0 | (ch >> 6));
                out += static_cast<char>(0x80 | (ch & 0x3F));
            }
            else
            {
                out += static_cast<char>(0xE0 | (ch >> 12));
                out += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (ch & 0x3F));
            }
        }
        return out;
    }

    std::string get_volume_label(TSK_FS_INFO *fs)
    {
        if (!fs)
            return "";

        // NTFS: read $VOLUME_NAME attribute (type 0x60) from $Volume (inode 3)
        if (fs->ftype == TSK_FS_TYPE_NTFS)
        {
            TSK_FS_FILE *vol_file = tsk_fs_file_open_meta(fs, nullptr, 3);
            if (vol_file)
            {
                const TSK_FS_ATTR *attr = tsk_fs_file_attr_get_type(
                    vol_file, TSK_FS_ATTR_TYPE_NTFS_VNAME, 0, 0);
                if (attr && (attr->flags & TSK_FS_ATTR_RES) &&
                    attr->rd.buf && attr->size > 0)
                {
                    std::string label = utf16le_to_utf8(
                        attr->rd.buf, static_cast<size_t>(attr->size));
                    tsk_fs_file_close(vol_file);
                    return label;
                }
                tsk_fs_file_close(vol_file);
            }
            return "";
        }

        // FAT/exFAT: volume label appears as a VIRT entry in the root directory
        if (TSK_FS_TYPE_ISFAT(fs->ftype))
        {
            TSK_FS_DIR *root = tsk_fs_dir_open(fs, "/");
            if (root)
            {
                for (size_t i = 0; i < root->names_used; ++i)
                {
                    const TSK_FS_NAME *entry = &root->names[i];
                    if (!entry || !entry->name)
                        continue;
                    if (entry->type == TSK_FS_NAME_TYPE_VIRT)
                    {
                        std::string label = entry->name;
                        tsk_fs_dir_close(root);
                        return label;
                    }
                }
                tsk_fs_dir_close(root);
            }
        }

        return "";
    }

    std::string generate_json_report(const std::string &image_path,
                                     const std::vector<PartitionReport> &partitions)
    {
        std::ostringstream out;

        out << "{\n";
        out << "  \"image\": \"" << escape_json_string(image_path) << "\",\n";
        out << "  \"partitions\": [\n";

        for (size_t pi = 0; pi < partitions.size(); ++pi)
        {
            const PartitionReport &part = partitions[pi];
            out << "    {\n";
            out << "      \"name\": \"" << escape_json_string(part.name) << "\",\n";
            out << "      \"desc\": \"" << escape_json_string(part.desc) << "\",\n";
            out << "      \"start_offset\": " << part.start_offset << ",\n";
            out << "      \"end_offset\": " << part.end_offset << ",\n";
            if (part.has_filesystem)
            {
                out << "      \"filesystem\": {\n";
                out << "        \"type\": \"" << escape_json_string(part.fs_type_name) << "\",\n";
                out << "        \"volume_label\": \"" << escape_json_string(part.volume_label) << "\",\n";
                out << "        \"block_size\": " << part.fs_block_size << ",\n";
                out << "        \"offset\": " << part.fs_offset_in_image << "\n";
                out << "      },\n";
            }
            out << "      \"files\": [\n";

            const std::vector<FileFragmentReport> &reports = part.files;
            for (size_t i = 0; i < reports.size(); ++i)
            {
                const FileFragmentReport &report = reports[i];
                out << "        {\n";
                out << "          \"filename\": \"" << escape_json_string(report.filename) << "\",\n";
                out << "          \"type\": \"" << report.type << "\",\n";
                out << "          \"is_fragmented\": " << (report.is_fragmented ? "true" : "false") << ",\n";
                out << "          \"is_deleted\": " << (report.is_deleted ? "true" : "false") << ",\n";
                out << "          \"size\": " << report.size << ",\n";
                out << "          \"fragments\": [\n";

                for (size_t j = 0; j < report.fragments.size(); ++j)
                {
                    const FragmentRange &fragment = report.fragments[j];
                    out << "            {\"start_offset\": " << fragment.start_offset
                        << ", \"end_offset\": " << fragment.end_offset << "}";
                    if (j + 1 < report.fragments.size())
                    {
                        out << ",";
                    }
                    out << "\n";
                }

                out << "          ]\n";
                out << "        }";
                if (i + 1 < reports.size())
                {
                    out << ",";
                }
                out << "\n";
            }

            out << "      ]\n";
            out << "    }";
            if (pi + 1 < partitions.size())
            {
                out << ",";
            }
            out << "\n";
        }

        out << "  ]\n";
        out << "}\n";

        return out.str();
    }

    void walk_directory_recursive(TSK_FS_INFO *fs,
                                  TSK_FS_DIR *dir,
                                  const std::string &path,
                                  std::vector<FileFragmentReport> &reports)
    {
        for (size_t i = 0; i < dir->names_used; ++i)
        {
            TSK_FS_NAME *entry = &dir->names[i];
            if (!entry || !entry->name || is_dot_entry(entry->name))
            {
                continue;
            }

            std::string full_path = join_path(path, entry->name);

            TSK_FS_FILE *file = tsk_fs_file_open_meta(fs, nullptr, entry->meta_addr);
            if (!file || !file->meta)
            {
                if (file)
                {
                    tsk_fs_file_close(file);
                }
                continue;
            }

            // Capture both files and directories
            if (file->meta->type == TSK_FS_META_TYPE_REG || file->meta->type == TSK_FS_META_TYPE_DIR)
            {
                reports.push_back(build_fragment_report(full_path, file, fs));
                if (entry->flags & TSK_FS_NAME_FLAG_UNALLOC)
                {
                    reports.back().is_deleted = true;
                }
            }

            // Recurse into subdirectories
            if (file->meta->type == TSK_FS_META_TYPE_DIR)
            {
                TSK_FS_DIR *subdir = tsk_fs_dir_open_meta(fs, entry->meta_addr);
                if (subdir)
                {
                    walk_directory_recursive(fs, subdir, full_path, reports);
                    tsk_fs_dir_close(subdir);
                }
            }

            tsk_fs_file_close(file);
        }
    }

    std::vector<FileFragmentReport> walk_filesystem(TSK_FS_INFO *fs)
    {
        std::vector<FileFragmentReport> reports;

        TSK_FS_DIR *root = tsk_fs_dir_open(fs, "/");
        if (!root)
        {
            return reports;
        }

        walk_directory_recursive(fs, root, "", reports);
        tsk_fs_dir_close(root);
        return reports;
    }

    std::vector<PartitionReport> analyze_image(TSK_IMG_INFO *img)
    {
        std::vector<PartitionReport> partitions;

        if (!img)
        {
            return partitions;
        }

        TSK_VS_INFO *vs = tsk_vs_open(img, 0, TSK_VS_TYPE_DETECT);
        if (vs)
        {
            size_t part_index = 0;
            for (const TSK_VS_PART_INFO *part = vs->part_list; part != nullptr; part = part->next)
            {
                if (!(part->flags & TSK_VS_PART_FLAG_ALLOC))
                {
                    continue;
                }

                PartitionReport prpt;
                prpt.name = "p" + std::to_string(part_index++);
                prpt.desc = (part->desc ? part->desc : "");
                prpt.start_offset = static_cast<uint64_t>(part->start) * vs->block_size;
                prpt.end_offset = prpt.start_offset + static_cast<uint64_t>(part->len) * vs->block_size - 1;

                TSK_FS_INFO *fs = tsk_fs_open_img(img, static_cast<TSK_OFF_T>(prpt.start_offset), TSK_FS_TYPE_DETECT);
                if (fs)
                {
                    prpt.has_filesystem = true;
                    prpt.fs_type_name = get_fs_type_name(fs->ftype);
                    prpt.volume_label = get_volume_label(fs);
                    prpt.fs_block_size = fs->block_size;
                    prpt.fs_offset_in_image = static_cast<uint64_t>(fs->offset);
                    prpt.files = walk_filesystem(fs);
                    tsk_fs_close(fs);
                }

                partitions.push_back(std::move(prpt));
            }
            tsk_vs_close(vs);
            return partitions;
        }

        // No volume system detected - treat entire image as a single partition
        TSK_FS_INFO *fs = tsk_fs_open_img(img, 0, TSK_FS_TYPE_DETECT);
        if (fs)
        {
            PartitionReport prpt;
            prpt.name = "p0";
            prpt.start_offset = 0;
            prpt.end_offset = static_cast<uint64_t>(img->size) > 0
                                  ? static_cast<uint64_t>(img->size) - 1
                                  : 0;
            prpt.has_filesystem = true;
            prpt.fs_type_name = get_fs_type_name(fs->ftype);
            prpt.volume_label = get_volume_label(fs);
            prpt.fs_block_size = fs->block_size;
            prpt.fs_offset_in_image = static_cast<uint64_t>(fs->offset);
            prpt.files = walk_filesystem(fs);
            tsk_fs_close(fs);
            partitions.push_back(std::move(prpt));
        }

        return partitions;
    }

    TSK_FS_FILE *find_file_by_path(TSK_FS_INFO *fs, const std::string &path)
    {
        if (!fs)
        {
            return nullptr;
        }

        std::string normalized = path;
        if (normalized.empty() || normalized[0] != '/')
        {
            normalized = "/" + normalized;
        }

        TSK_FS_FILE *file = tsk_fs_file_open(fs, nullptr, normalized.c_str());
        return file;
    }

    bool extract_file_to_buffer(TSK_FS_FILE *file, uint8_t *output_buffer, size_t *output_size)
    {
        if (!file || !file->meta || file->meta->type != TSK_FS_META_TYPE_REG)
        {
            return false;
        }

        const uint64_t file_size = static_cast<uint64_t>(file->meta->size);

        if (file_size > *output_size)
        {
            *output_size = file_size;
            return false;
        }

        const size_t read_chunk = 65536;
        char buffer[read_chunk];
        uint64_t bytes_read = 0;

        while (bytes_read < file_size)
        {
            const size_t to_read = std::min(static_cast<size_t>(file_size - bytes_read), read_chunk);
            const ssize_t num_read = tsk_fs_file_read(file, bytes_read, buffer, to_read, TSK_FS_FILE_READ_FLAG_NONE);

            if (num_read <= 0)
            {
                return false;
            }

            std::memcpy(output_buffer + bytes_read, buffer, num_read);
            bytes_read += num_read;
        }

        *output_size = file_size;
        return true;
    }
}

// C API Implementation - Simple string in, string out

extern "C"
{
    char *libtusk_analyze(const char *image_path)
    {
        if (!image_path)
        {
            return nullptr;
        }

#ifdef TSK_WIN32
        std::wstring wpath(image_path, image_path + strlen(image_path));
        TSK_IMG_INFO *img = tsk_img_open_sing(wpath.c_str(), TSK_IMG_TYPE_DETECT, 0);
#else
        TSK_IMG_INFO *img = tsk_img_open_sing(image_path, TSK_IMG_TYPE_DETECT, 0);
#endif
        if (!img)
        {
            return nullptr;
        }

        std::vector<PartitionReport> partitions = analyze_image(img);
        std::string json = generate_json_report(image_path, partitions);

        tsk_img_close(img);

        // Allocate and return JSON string
        char *result = static_cast<char *>(malloc(json.size() + 1));
        if (result)
        {
            std::memcpy(result, json.c_str(), json.size());
            result[json.size()] = '\0';
        }

        return result;
    }

    void libtusk_free(char *ptr)
    {
        if (ptr)
        {
            free(ptr);
        }
    }
}
