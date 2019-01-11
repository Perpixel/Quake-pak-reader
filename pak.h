#pragma once

#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>


namespace pakutils
{
    namespace pakformat
    {
        constexpr int MAX_FILES_IN_PACK = 2048;
        constexpr int ITEM_NAME_LENGTH = 56;

        struct PakHeader
        {
            char    id[4];  // must be PACK
            int     offset;
            int     size;
        };

        struct PakFileDesc
        {
            char    name[ITEM_NAME_LENGTH];
            int     offset;     // Position in pak file
            int     size;       // File length
        };
    }

    class CPakLoader
    {
    public:

        CPakLoader():
            valid_pak_(false),
            num_files_(0)
        {
            /* do nothing */
        }

        ~CPakLoader()
        {
            Close();
        }

        bool Open(const std::string& file);
        void Close();
        const bool isValid() const { return valid_pak_ && (num_files_ > 0); };
        const size_t GetNumFiles() const { return num_files_; }
        std::string GetFileById(size_t id, std::vector<char>& out);

    private:

        // vars
        std::ifstream pak_stream_;
        bool valid_pak_;
        size_t num_files_;
        std::vector<pakformat::PakFileDesc> file_desc_list_;

        // methods
        template<typename T>
        std::istream& PakRead(std::istream& stream, T& out);
    };

    bool CPakLoader::Open(const std::string& file)
    {
        Close(); // reset loader

        pakformat::PakHeader header;

        pak_stream_.open(file.c_str(), std::ios::binary);
        PakRead(pak_stream_, header);

        if (!pak_stream_.is_open())
        {
            // unable to open file.
            return false;
        }

        if (   header.id[0] != 'P'
            || header.id[1] != 'A'
            || header.id[2] != 'C'
            || header.id[3] != 'K')
        {
            // header is not valid
            return false;
        }

        num_files_ = header.size / sizeof(pakformat::PakFileDesc);

        if (num_files_ > pakformat::MAX_FILES_IN_PACK)
        {
            // Too many file
            return false;
        }

        // passed all validation
        valid_pak_ = true;

        // go to files
        pak_stream_.seekg(header.offset);

        for (size_t i = 0; i < num_files_; i++)
        {
            pakformat::PakFileDesc file;
            PakRead(pak_stream_, file);

            if (file.size < 0)
            {
                assert(file.size > 0);
            }

            file_desc_list_.push_back(file);
        }

        return true;
    }

    void CPakLoader::Close()
    {
        // clean up existing data and release ressources
        num_files_ = 0;
        valid_pak_ = false;
        file_desc_list_.clear();

        if (pak_stream_.is_open())
        {
            pak_stream_.close();
        }
    }

    std::string CPakLoader::GetFileById(size_t id, std::vector<char>& out)
    {
        if (file_desc_list_.size() < id)
        {
            return "";
        }

        pakformat::PakFileDesc& file = file_desc_list_.at(id);
        pak_stream_.seekg(file.offset);
        
        std::istreambuf_iterator<char> start(pak_stream_);
        std::copy_n(start, file.size, std::back_inserter(out));

        return file.name;
    }

    template<typename T>
    std::istream& CPakLoader::PakRead(std::istream& stream, T& out)
    {
        return stream.read(reinterpret_cast<char*>(&out), sizeof(T));
    }
}