#pragma once

// =============================================================================
// IBM IMS Emulation Enterprise - SMF Record Generation
// Version: 3.6.2
// =============================================================================
//
// System Management Facilities (SMF) record generation for auditing,
// performance monitoring, and accounting. Supports common SMF record types.
//
// Copyright (c) 2025 Bennie Shearer
// MIT License - See LICENSE file for details
// =============================================================================

#include "types.hpp"
#include "time_utils.hpp"

namespace ims::smf {

// =============================================================================
// SMF Record Types
// =============================================================================

enum class SmfRecordType : UInt8 {
    TYPE_0   = 0,    // IPL
    TYPE_4   = 4,    // Step termination
    TYPE_5   = 5,    // Job termination
    TYPE_14  = 14,   // Dataset open (input)
    TYPE_15  = 15,   // Dataset open (output)
    TYPE_17  = 17,   // Scratch data set
    TYPE_18  = 18,   // Rename data set
    TYPE_30  = 30,   // Common address space work
    TYPE_42  = 42,   // DFSMS statistics
    TYPE_60  = 60,   // VSAM statistics
    TYPE_62  = 62,   // VSAM component statistics
    TYPE_64  = 64,   // VSAM extended statistics
    TYPE_70  = 70,   // RMF processor activity
    TYPE_71  = 71,   // RMF paging activity
    TYPE_72  = 72,   // RMF workload activity
    TYPE_74  = 74,   // RMF device activity
    TYPE_80  = 80,   // RACF events
    TYPE_83  = 83,   // RACF statistics
    TYPE_90  = 90,   // System status
    TYPE_100 = 100,  // DB2 statistics
    TYPE_101 = 101,  // DB2 accounting
    TYPE_102 = 102,  // DB2 performance
    TYPE_110 = 110,  // CICS statistics
    TYPE_130 = 130,  // TCP/IP statistics
    TYPE_199 = 199,  // IMS Emulation custom record
};

inline String smf_type_to_string(SmfRecordType type) {
    switch (type) {
        case SmfRecordType::TYPE_0:   return "IPL";
        case SmfRecordType::TYPE_4:   return "Step Termination";
        case SmfRecordType::TYPE_5:   return "Job Termination";
        case SmfRecordType::TYPE_14:  return "Dataset Open (Input)";
        case SmfRecordType::TYPE_15:  return "Dataset Open (Output)";
        case SmfRecordType::TYPE_30:  return "Address Space Work";
        case SmfRecordType::TYPE_42:  return "DFSMS Statistics";
        case SmfRecordType::TYPE_60:  return "VSAM Statistics";
        case SmfRecordType::TYPE_80:  return "RACF Events";
        case SmfRecordType::TYPE_199: return "IMS Emulation";
        default: return std::format("Type {}", static_cast<int>(type));
    }
}

// =============================================================================
// SMF Header (Standard Format)
// =============================================================================

#pragma pack(push, 1)
struct SmfHeader {
    UInt16 length;           // Record length (including header)
    UInt16 segment_desc;     // Segment descriptor
    UInt8  flag;             // System indicator flags
    UInt8  record_type;      // SMF record type
    UInt32 time;             // Time of day (hundredths of second since midnight)
    UInt32 date;             // Date (0cyydddF packed)
    char   system_id[4];     // System identification (SID)
    
    SmfHeader() {
        length = sizeof(SmfHeader);
        segment_desc = 0;
        flag = 0;
        record_type = 0;
        time = 0;
        date = 0;
        std::memset(system_id, ' ', 4);
    }
    
    void set_current_time() {
        auto now = SystemClock::now();
        auto tt = SystemClock::to_time_t(now);
        auto tm = *std::localtime(&tt);
        
        // Time in hundredths of seconds since midnight
        time = static_cast<UInt32>((tm.tm_hour * 360000) + 
                                   (tm.tm_min * 6000) + 
                                   (tm.tm_sec * 100));
        
        // Date in 0cyydddF format (packed decimal)
        int year = tm.tm_year;  // Years since 1900
        int century = (year >= 100) ? 1 : 0;
        int yy = year % 100;
        int ddd = tm.tm_yday + 1;
        
        date = static_cast<UInt32>((century << 28) | 
                                   ((yy / 10) << 24) | 
                                   ((yy % 10) << 20) |
                                   ((ddd / 100) << 16) |
                                   (((ddd / 10) % 10) << 12) |
                                   ((ddd % 10) << 8) |
                                   0x0F);
    }
    
    void set_system_id(StringView sid) {
        Size len = std::min(sid.length(), Size(4));
        std::memset(system_id, ' ', 4);
        std::memcpy(system_id, sid.data(), len);
    }
};
#pragma pack(pop)

// =============================================================================
// SMF Type 14/15 - Dataset Open Records
// =============================================================================

#pragma pack(push, 1)
struct SmfType14_15 {
    SmfHeader header;
    char      jobname[8];
    char      stepname[8];
    char      ddname[8];
    char      dsname[44];
    char      volser[6];
    UInt16    device_type;
    UInt8     open_type;      // 1=input, 2=output, 3=update
    UInt8     dsorg;
    UInt32    block_count;
    UInt64    byte_count;
    
    SmfType14_15(bool is_input = true) {
        std::memset(this, 0, sizeof(*this));
        header.record_type = static_cast<UInt8>(is_input ? SmfRecordType::TYPE_14 : SmfRecordType::TYPE_15);
        header.length = sizeof(SmfType14_15);
        header.set_current_time();
        std::memset(jobname, ' ', sizeof(jobname));
        std::memset(stepname, ' ', sizeof(stepname));
        std::memset(ddname, ' ', sizeof(ddname));
        std::memset(dsname, ' ', sizeof(dsname));
        std::memset(volser, ' ', sizeof(volser));
        open_type = is_input ? 1 : 2;
    }
    
    void set_jobname(StringView name) {
        Size len = std::min(name.length(), Size(8));
        std::memset(jobname, ' ', 8);
        std::memcpy(jobname, name.data(), len);
    }
    
    void set_dsname(StringView name) {
        Size len = std::min(name.length(), Size(44));
        std::memset(dsname, ' ', 44);
        std::memcpy(dsname, name.data(), len);
    }
    
    void set_ddname(StringView name) {
        Size len = std::min(name.length(), Size(8));
        std::memset(ddname, ' ', 8);
        std::memcpy(ddname, name.data(), len);
    }
    
    void set_volser(StringView vol) {
        Size len = std::min(vol.length(), Size(6));
        std::memset(volser, ' ', 6);
        std::memcpy(volser, vol.data(), len);
    }
};
#pragma pack(pop)

// =============================================================================
// SMF Type 60 - VSAM Statistics
// =============================================================================

#pragma pack(push, 1)
struct SmfType60 {
    SmfHeader header;
    char      cluster_name[44];
    UInt8     vsam_type;      // 1=KSDS, 2=ESDS, 3=RRDS, 4=LDS
    UInt8     reserved1;
    UInt16    reserved2;
    UInt64    get_requests;
    UInt64    put_requests;
    UInt64    erase_requests;
    UInt64    point_requests;
    UInt64    records_read;
    UInt64    records_written;
    UInt64    ci_splits;
    UInt64    ca_splits;
    UInt64    index_lookups;
    UInt64    buffers_used;
    
    SmfType60() {
        std::memset(this, 0, sizeof(*this));
        header.record_type = static_cast<UInt8>(SmfRecordType::TYPE_60);
        header.length = sizeof(SmfType60);
        header.set_current_time();
        std::memset(cluster_name, ' ', sizeof(cluster_name));
    }
    
    void set_cluster_name(StringView name) {
        Size len = std::min(name.length(), Size(44));
        std::memset(cluster_name, ' ', 44);
        std::memcpy(cluster_name, name.data(), len);
    }
};
#pragma pack(pop)

// =============================================================================
// SMF Type 199 - IMS Emulation Custom Record
// =============================================================================

enum class ImsSmfSubtype : UInt8 {
    TRANSACTION = 1,
    DATABASE_ACCESS = 2,
    CATALOG_OPERATION = 3,
    SECURITY_EVENT = 4,
    PERFORMANCE = 5,
    ERROR = 6
};

#pragma pack(push, 1)
struct SmfType199 {
    SmfHeader header;
    UInt8     subtype;
    UInt8     reserved1;
    UInt16    reserved2;
    char      transaction_id[8];
    char      program_name[8];
    char      database_name[8];
    char      segment_name[8];
    char      user_id[8];
    UInt32    processing_time_us;  // Microseconds
    UInt32    cpu_time_us;
    UInt64    bytes_read;
    UInt64    bytes_written;
    UInt32    dli_calls;
    UInt32    status_code;
    char      error_message[80];
    
    SmfType199() {
        std::memset(this, 0, sizeof(*this));
        header.record_type = static_cast<UInt8>(SmfRecordType::TYPE_199);
        header.length = sizeof(SmfType199);
        header.set_current_time();
        std::memset(transaction_id, ' ', sizeof(transaction_id));
        std::memset(program_name, ' ', sizeof(program_name));
        std::memset(database_name, ' ', sizeof(database_name));
        std::memset(segment_name, ' ', sizeof(segment_name));
        std::memset(user_id, ' ', sizeof(user_id));
        std::memset(error_message, ' ', sizeof(error_message));
    }
    
    void set_transaction_id(StringView id) {
        Size len = std::min(id.length(), Size(8));
        std::memset(transaction_id, ' ', 8);
        std::memcpy(transaction_id, id.data(), len);
    }
    
    void set_user_id(StringView id) {
        Size len = std::min(id.length(), Size(8));
        std::memset(user_id, ' ', 8);
        std::memcpy(user_id, id.data(), len);
    }
    
    void set_database_name(StringView name) {
        Size len = std::min(name.length(), Size(8));
        std::memset(database_name, ' ', 8);
        std::memcpy(database_name, name.data(), len);
    }
    
    void set_error_message(StringView msg) {
        Size len = std::min(msg.length(), Size(80));
        std::memset(error_message, ' ', 80);
        std::memcpy(error_message, msg.data(), len);
    }
};
#pragma pack(pop)

// =============================================================================
// SMF Writer
// =============================================================================

class SmfWriter {
private:
    Path output_path_;
    std::ofstream file_;
    mutable Mutex mutex_;
    UInt64 records_written_{0};
    bool is_open_{false};
    
public:
    SmfWriter() = default;
    
    explicit SmfWriter(const Path& path) : output_path_(path) {
        open(path);
    }
    
    ~SmfWriter() {
        close();
    }
    
    bool open(const Path& path) {
        LockGuard<Mutex> lock(mutex_);
        if (is_open_) close();
        
        output_path_ = path;
        file_.open(path, std::ios::binary | std::ios::app);
        is_open_ = file_.is_open();
        return is_open_;
    }
    
    void close() {
        LockGuard<Mutex> lock(mutex_);
        if (is_open_) {
            file_.close();
            is_open_ = false;
        }
    }
    
    bool is_open() const { return is_open_; }
    
    template<typename T>
    bool write(const T& record) {
        LockGuard<Mutex> lock(mutex_);
        if (!is_open_) return false;
        
        file_.write(reinterpret_cast<const char*>(&record), sizeof(T));
        ++records_written_;
        return file_.good();
    }
    
    UInt64 records_written() const {
        LockGuard<Mutex> lock(mutex_);
        return records_written_;
    }
    
    void flush() {
        LockGuard<Mutex> lock(mutex_);
        if (is_open_) file_.flush();
    }
};

// =============================================================================
// SMF Reader
// =============================================================================

class SmfReader {
private:
    Path input_path_;
    std::ifstream file_;
    bool is_open_{false};
    
public:
    SmfReader() = default;
    
    explicit SmfReader(const Path& path) : input_path_(path) {
        open(path);
    }
    
    bool open(const Path& path) {
        if (is_open_) close();
        
        input_path_ = path;
        file_.open(path, std::ios::binary);
        is_open_ = file_.is_open();
        return is_open_;
    }
    
    void close() {
        if (is_open_) {
            file_.close();
            is_open_ = false;
        }
    }
    
    bool is_open() const { return is_open_; }
    
    /// Read next record header to determine type
    Optional<SmfHeader> read_header() {
        if (!is_open_ || file_.eof()) return std::nullopt;
        
        SmfHeader header;
        file_.read(reinterpret_cast<char*>(&header), sizeof(SmfHeader));
        
        if (file_.gcount() != sizeof(SmfHeader)) {
            return std::nullopt;
        }
        
        // Seek back to re-read full record
        file_.seekg(-static_cast<std::streamoff>(sizeof(SmfHeader)), std::ios::cur);
        
        return header;
    }
    
    template<typename T>
    Optional<T> read() {
        if (!is_open_ || file_.eof()) return std::nullopt;
        
        T record;
        file_.read(reinterpret_cast<char*>(&record), sizeof(T));
        
        if (file_.gcount() != sizeof(T)) {
            return std::nullopt;
        }
        
        return record;
    }
    
    /// Skip to next record
    bool skip() {
        auto header_opt = read_header();
        if (!header_opt) return false;
        
        file_.seekg(header_opt->length, std::ios::cur);
        return !file_.eof();
    }
    
    bool eof() const { return !is_open_ || file_.eof(); }
};

// =============================================================================
// Global SMF Writer
// =============================================================================

inline SmfWriter& global_smf_writer() {
    static SmfWriter writer;
    return writer;
}

// =============================================================================
// Convenience Functions
// =============================================================================

inline void record_dataset_open(StringView dsname, StringView ddname, 
                                StringView jobname, bool is_input) {
    SmfType14_15 record(is_input);
    record.set_dsname(dsname);
    record.set_ddname(ddname);
    record.set_jobname(jobname);
    global_smf_writer().write(record);
}

inline void record_vsam_stats(StringView cluster, UInt8 vsam_type,
                              UInt64 gets, UInt64 puts, UInt64 erases) {
    SmfType60 record;
    record.set_cluster_name(cluster);
    record.vsam_type = vsam_type;
    record.get_requests = gets;
    record.put_requests = puts;
    record.erase_requests = erases;
    global_smf_writer().write(record);
}

inline void record_ims_transaction(StringView tran_id, StringView user_id,
                                   StringView database, UInt32 processing_time_us,
                                   UInt32 dli_calls, UInt32 status) {
    SmfType199 record;
    record.subtype = static_cast<UInt8>(ImsSmfSubtype::TRANSACTION);
    record.set_transaction_id(tran_id);
    record.set_user_id(user_id);
    record.set_database_name(database);
    record.processing_time_us = processing_time_us;
    record.dli_calls = dli_calls;
    record.status_code = status;
    global_smf_writer().write(record);
}

} // namespace ims::smf
