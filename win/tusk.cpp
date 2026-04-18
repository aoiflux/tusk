#include <tsk/libtsk.h>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstring>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

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
        std::string type; // "file" or "directory"
        bool is_fragmented = false;
        uint64_t size = 0;
        std::vector<FragmentRange> fragments;
    };

    struct PartitionReport
    {
        std::string name;
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

    bool try_parse_u64(const std::string &text, uint64_t &value)
    {
        try
        {
            size_t parsed = 0;
            value = std::stoull(text, &parsed, 10);
            return parsed == text.size();
        }
        catch (const std::exception &)
        {
            return false;
        }
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

    bool write_json_report(const std::string &output_path,
                           const std::string &image_path,
                           const std::vector<PartitionReport> &partitions)
    {
        std::ofstream out(output_path);
        if (!out)
        {
            return false;
        }

        out << "{\n";
        out << "  \"image\": \"" << escape_json_string(image_path) << "\",\n";
        out << "  \"partitions\": [\n";

        for (size_t pi = 0; pi < partitions.size(); ++pi)
        {
            const PartitionReport &part = partitions[pi];
            out << "    {\n";
            out << "      \"name\": \"" << escape_json_string(part.name) << "\",\n";
            out << "      \"start_offset\": " << part.start_offset << ",\n";
            out << "      \"end_offset\": " << part.end_offset << ",\n";
            if (part.has_filesystem)
            {
                out << "      \"filesystem\": {\n";
                out << "        \"type\": \"" << escape_json_string(part.fs_type_name) << "\",\n";
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
        return true;
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
            std::cout << "Failed to open root directory\n";
            return reports;
        }

        walk_directory_recursive(fs, root, "", reports);
        tsk_fs_dir_close(root);
        return reports;
    }

    void print_partitions(TSK_IMG_INFO *img)
    {
        TSK_VS_INFO *vs = tsk_vs_open(img, 0, TSK_VS_TYPE_DETECT);
        if (!vs)
        {
            std::cout << "No partition table detected\n";
            return;
        }

        std::cout << "Partition Table Detected\n";

        for (size_t i = 0; i < vs->part_count; i++)
        {
            const TSK_VS_PART_INFO *p = &vs->part_list[i];

            uint64_t start_bytes = p->start * vs->block_size;
            uint64_t size_bytes = p->len * vs->block_size;

            std::cout << "  [" << i << "] "
                      << "Start: " << start_bytes
                      << "  Size: " << size_bytes
                      << "  Desc: " << (p->desc ? p->desc : "")
                      << "\n";
        }

        tsk_vs_close(vs);
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
            for (size_t i = 0; i < static_cast<size_t>(vs->part_count); ++i)
            {
                const TSK_VS_PART_INFO *part = &vs->part_list[i];
                if (!part || !(part->flags & TSK_VS_PART_FLAG_ALLOC))
                {
                    continue;
                }

                PartitionReport prpt;
                prpt.name = "p" + std::to_string(part_index++);
                prpt.start_offset = static_cast<uint64_t>(part->start) * vs->block_size;
                prpt.end_offset = prpt.start_offset + static_cast<uint64_t>(part->len) * vs->block_size - 1;

                TSK_FS_INFO *fs = tsk_fs_open_img(img, static_cast<TSK_OFF_T>(prpt.start_offset), TSK_FS_TYPE_DETECT);
                if (fs)
                {
                    prpt.has_filesystem = true;
                    prpt.fs_type_name = get_fs_type_name(fs->ftype);
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
            prpt.fs_block_size = fs->block_size;
            prpt.fs_offset_in_image = static_cast<uint64_t>(fs->offset);
            prpt.files = walk_filesystem(fs);
            tsk_fs_close(fs);
            partitions.push_back(std::move(prpt));
        }

        return partitions;
    }

    bool get_partition_offset(TSK_IMG_INFO *img, uint64_t allocated_index, uint64_t &offset)
    {
        offset = 0;
        if (!img)
        {
            return false;
        }

        TSK_VS_INFO *vs = tsk_vs_open(img, 0, TSK_VS_TYPE_DETECT);
        if (!vs)
        {
            // No VS: p0 is the whole image at offset 0
            if (allocated_index == 0)
            {
                offset = 0;
                return true;
            }
            return false;
        }

        uint64_t count = 0;
        for (size_t i = 0; i < static_cast<size_t>(vs->part_count); ++i)
        {
            const TSK_VS_PART_INFO *part = &vs->part_list[i];
            if (!part || !(part->flags & TSK_VS_PART_FLAG_ALLOC))
            {
                continue;
            }

            if (count == allocated_index)
            {
                offset = static_cast<uint64_t>(part->start) * vs->block_size;
                tsk_vs_close(vs);
                return true;
            }
            ++count;
        }

        tsk_vs_close(vs);
        return false;
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

    bool extract_file_to_disk(TSK_FS_FILE *file, const std::string &output_path)
    {
        if (!file || !file->meta || file->meta->type != TSK_FS_META_TYPE_REG)
        {
            return false;
        }

        const uint64_t file_size = static_cast<uint64_t>(file->meta->size);
        const size_t read_chunk = 65536;
        char buffer[read_chunk];

        std::ofstream out(output_path, std::ios::binary);
        if (!out)
        {
            return false;
        }

        uint64_t bytes_read = 0;
        while (bytes_read < file_size)
        {
            const size_t to_read = std::min(static_cast<size_t>(file_size - bytes_read), read_chunk);
            const ssize_t num_read = tsk_fs_file_read(file, bytes_read, buffer, to_read, TSK_FS_FILE_READ_FLAG_NONE);

            if (num_read <= 0)
            {
                return false;
            }

            out.write(buffer, num_read);
            if (!out)
            {
                return false;
            }

            bytes_read += num_read;
        }

        return true;
    }

} // namespace

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "Usage:\n";
        std::cout << "  tsktool <raw_image> [output_json]\n";
        std::cout << "  tsktool <raw_image> extract <file_path> [output_file] [--partition <index>]\n";
        return 1;
    }

    const char *image_path = argv[1];

    // Check if this is an extract command
    bool is_extract = (argc >= 3 && std::string(argv[2]) == "extract");

    if (is_extract)
    {
        if (argc < 4)
        {
            std::cout << "Usage: tsktool <raw_image> extract <file_path> [output_file] [--partition <index>]\n";
            return 1;
        }

        const char *extract_path = argv[3];
        std::string output_file = "";
        bool output_file_set = false;
        bool has_partition_index = false;
        uint64_t partition_index = 0;

        for (int i = 4; i < argc; ++i)
        {
            std::string arg = argv[i];

            if (arg == "--partition")
            {
                if (i + 1 >= argc || !try_parse_u64(argv[i + 1], partition_index))
                {
                    std::cout << "Invalid value for --partition\n";
                    return 1;
                }
                has_partition_index = true;
                ++i;
                continue;
            }

            if (!output_file_set)
            {
                output_file = arg;
                output_file_set = true;
                continue;
            }

            std::cout << "Unknown argument: " << arg << "\n";
            return 1;
        }

        // Default output filename
        if (output_file.empty())
        {
            const char *last_slash = std::strrchr(extract_path, '/');
            output_file = (last_slash != nullptr) ? std::string(last_slash + 1) : std::string(extract_path);
        }

        TSK_IMG_INFO *img = tsk_img_open_utf8_sing(image_path, TSK_IMG_TYPE_DETECT, 0);
        if (!img)
        {
            std::cout << "Failed to open image\n";
            return 1;
        }

        uint64_t fs_offset = 0;

        if (has_partition_index)
        {
            if (!get_partition_offset(img, partition_index, fs_offset))
            {
                std::cout << "Failed to resolve usable offset from partition index " << partition_index << "\n";
                tsk_img_close(img);
                return 1;
            }
        }

        TSK_FS_INFO *fs = tsk_fs_open_img(img, fs_offset, TSK_FS_TYPE_DETECT);
        if (!fs)
        {
            std::cout << "No filesystem detected at offset " << fs_offset << "\n";
            tsk_img_close(img);
            return 1;
        }

        TSK_FS_FILE *file = find_file_by_path(fs, extract_path);
        if (!file)
        {
            std::cout << "File not found: " << extract_path << "\n";
            tsk_fs_close(fs);
            tsk_img_close(img);
            return 1;
        }

        if (file->meta->type != TSK_FS_META_TYPE_REG)
        {
            std::cout << "Not a regular file: " << extract_path << "\n";
            tsk_fs_file_close(file);
            tsk_fs_close(fs);
            tsk_img_close(img);
            return 1;
        }

        if (!extract_file_to_disk(file, output_file))
        {
            std::cout << "Failed to extract file to: " << output_file << "\n";
            tsk_fs_file_close(file);
            tsk_fs_close(fs);
            tsk_img_close(img);
            return 1;
        }

        std::cout << "File extracted to: " << output_file << " (" << file->meta->size << " bytes)\n";

        tsk_fs_file_close(file);
        tsk_fs_close(fs);
        tsk_img_close(img);
        return 0;
    }

    // Default: generate JSON report
    std::string output_json = "report.json";
    bool output_json_set = false;

    for (int i = 2; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (!output_json_set)
        {
            output_json = arg;
            output_json_set = true;
            continue;
        }

        std::cout << "Unknown argument: " << arg << "\n";
        std::cout << "Usage:\n";
        std::cout << "  tsktool <raw_image> [output_json]\n";
        std::cout << "  tsktool <raw_image> extract <file_path> [output_file] [--partition <index>]\n";
        return 1;
    }

    TSK_IMG_INFO *img = tsk_img_open_utf8_sing(image_path, TSK_IMG_TYPE_DETECT, 0);
    if (!img)
    {
        std::cout << "Failed to open image\n";
        return 1;
    }

    std::cout << "Image size: " << img->size << " bytes\n";

    print_partitions(img);

    std::vector<PartitionReport> partitions = analyze_image(img);

    if (!write_json_report(output_json, image_path, partitions))
    {
        std::cout << "Failed to write JSON report: " << output_json << "\n";
    }
    else
    {
        size_t total_files = 0;
        for (const PartitionReport &p : partitions)
        {
            total_files += p.files.size();
        }
        std::cout << "JSON report written to: " << output_json
                  << " (partitions=" << partitions.size()
                  << ", files=" << total_files << ")\n";
    }

    tsk_img_close(img);
    return 0;
}