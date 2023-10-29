// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: filearchive.proto

#include "filearchive.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>

PROTOBUF_PRAGMA_INIT_SEG

namespace _pb = ::PROTOBUF_NAMESPACE_ID;
namespace _pbi = _pb::internal;

namespace Odb {
namespace Lib {
namespace Protobuf {
PROTOBUF_CONSTEXPR FileArchive_StepsByNameEntry_DoNotUse::FileArchive_StepsByNameEntry_DoNotUse(
    ::_pbi::ConstantInitialized) {}
struct FileArchive_StepsByNameEntry_DoNotUseDefaultTypeInternal {
  PROTOBUF_CONSTEXPR FileArchive_StepsByNameEntry_DoNotUseDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~FileArchive_StepsByNameEntry_DoNotUseDefaultTypeInternal() {}
  union {
    FileArchive_StepsByNameEntry_DoNotUse _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 FileArchive_StepsByNameEntry_DoNotUseDefaultTypeInternal _FileArchive_StepsByNameEntry_DoNotUse_default_instance_;
PROTOBUF_CONSTEXPR FileArchive::FileArchive(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_._has_bits_)*/{}
  , /*decltype(_impl_._cached_size_)*/{}
  , /*decltype(_impl_.stepsbyname_)*/{::_pbi::ConstantInitialized()}
  , /*decltype(_impl_.miscinfofile_)*/nullptr
  , /*decltype(_impl_.matrixfile_)*/nullptr
  , /*decltype(_impl_.standardfontsfile_)*/nullptr} {}
struct FileArchiveDefaultTypeInternal {
  PROTOBUF_CONSTEXPR FileArchiveDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~FileArchiveDefaultTypeInternal() {}
  union {
    FileArchive _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 FileArchiveDefaultTypeInternal _FileArchive_default_instance_;
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
static ::_pb::Metadata file_level_metadata_filearchive_2eproto[2];
static constexpr ::_pb::EnumDescriptor const** file_level_enum_descriptors_filearchive_2eproto = nullptr;
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_filearchive_2eproto = nullptr;

const uint32_t TableStruct_filearchive_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::FileArchive_StepsByNameEntry_DoNotUse, _has_bits_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::FileArchive_StepsByNameEntry_DoNotUse, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::FileArchive_StepsByNameEntry_DoNotUse, key_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::FileArchive_StepsByNameEntry_DoNotUse, value_),
  0,
  1,
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::FileArchive, _impl_._has_bits_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::FileArchive, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::FileArchive, _impl_.stepsbyname_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::FileArchive, _impl_.miscinfofile_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::FileArchive, _impl_.matrixfile_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::FileArchive, _impl_.standardfontsfile_),
  ~0u,
  0,
  1,
  2,
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, 8, -1, sizeof(::Odb::Lib::Protobuf::FileArchive_StepsByNameEntry_DoNotUse)},
  { 10, 20, -1, sizeof(::Odb::Lib::Protobuf::FileArchive)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::Odb::Lib::Protobuf::_FileArchive_StepsByNameEntry_DoNotUse_default_instance_._instance,
  &::Odb::Lib::Protobuf::_FileArchive_default_instance_._instance,
};

const char descriptor_table_protodef_filearchive_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\021filearchive.proto\022\020Odb.Lib.Protobuf\032\023s"
  "tepdirectory.proto\032\022miscinfofile.proto\032\020"
  "matrixfile.proto\032\027standardfontsfile.prot"
  "o\"\224\003\n\013FileArchive\022C\n\013stepsByName\030\001 \003(\0132."
  ".Odb.Lib.Protobuf.FileArchive.StepsByNam"
  "eEntry\0229\n\014miscInfoFile\030\002 \001(\0132\036.Odb.Lib.P"
  "rotobuf.MiscInfoFileH\000\210\001\001\0225\n\nmatrixFile\030"
  "\003 \001(\0132\034.Odb.Lib.Protobuf.MatrixFileH\001\210\001\001"
  "\022C\n\021standardFontsFile\030\004 \001(\0132#.Odb.Lib.Pr"
  "otobuf.StandardFontsFileH\002\210\001\001\032S\n\020StepsBy"
  "NameEntry\022\013\n\003key\030\001 \001(\t\022.\n\005value\030\002 \001(\0132\037."
  "Odb.Lib.Protobuf.StepDirectory:\0028\001B\017\n\r_m"
  "iscInfoFileB\r\n\013_matrixFileB\024\n\022_standardF"
  "ontsFileb\006proto3"
  ;
static const ::_pbi::DescriptorTable* const descriptor_table_filearchive_2eproto_deps[4] = {
  &::descriptor_table_matrixfile_2eproto,
  &::descriptor_table_miscinfofile_2eproto,
  &::descriptor_table_standardfontsfile_2eproto,
  &::descriptor_table_stepdirectory_2eproto,
};
static ::_pbi::once_flag descriptor_table_filearchive_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_filearchive_2eproto = {
    false, false, 536, descriptor_table_protodef_filearchive_2eproto,
    "filearchive.proto",
    &descriptor_table_filearchive_2eproto_once, descriptor_table_filearchive_2eproto_deps, 4, 2,
    schemas, file_default_instances, TableStruct_filearchive_2eproto::offsets,
    file_level_metadata_filearchive_2eproto, file_level_enum_descriptors_filearchive_2eproto,
    file_level_service_descriptors_filearchive_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_filearchive_2eproto_getter() {
  return &descriptor_table_filearchive_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_filearchive_2eproto(&descriptor_table_filearchive_2eproto);
namespace Odb {
namespace Lib {
namespace Protobuf {

// ===================================================================

FileArchive_StepsByNameEntry_DoNotUse::FileArchive_StepsByNameEntry_DoNotUse() {}
FileArchive_StepsByNameEntry_DoNotUse::FileArchive_StepsByNameEntry_DoNotUse(::PROTOBUF_NAMESPACE_ID::Arena* arena)
    : SuperType(arena) {}
void FileArchive_StepsByNameEntry_DoNotUse::MergeFrom(const FileArchive_StepsByNameEntry_DoNotUse& other) {
  MergeFromInternal(other);
}
::PROTOBUF_NAMESPACE_ID::Metadata FileArchive_StepsByNameEntry_DoNotUse::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_filearchive_2eproto_getter, &descriptor_table_filearchive_2eproto_once,
      file_level_metadata_filearchive_2eproto[0]);
}

// ===================================================================

class FileArchive::_Internal {
 public:
  using HasBits = decltype(std::declval<FileArchive>()._impl_._has_bits_);
  static const ::Odb::Lib::Protobuf::MiscInfoFile& miscinfofile(const FileArchive* msg);
  static void set_has_miscinfofile(HasBits* has_bits) {
    (*has_bits)[0] |= 1u;
  }
  static const ::Odb::Lib::Protobuf::MatrixFile& matrixfile(const FileArchive* msg);
  static void set_has_matrixfile(HasBits* has_bits) {
    (*has_bits)[0] |= 2u;
  }
  static const ::Odb::Lib::Protobuf::StandardFontsFile& standardfontsfile(const FileArchive* msg);
  static void set_has_standardfontsfile(HasBits* has_bits) {
    (*has_bits)[0] |= 4u;
  }
};

const ::Odb::Lib::Protobuf::MiscInfoFile&
FileArchive::_Internal::miscinfofile(const FileArchive* msg) {
  return *msg->_impl_.miscinfofile_;
}
const ::Odb::Lib::Protobuf::MatrixFile&
FileArchive::_Internal::matrixfile(const FileArchive* msg) {
  return *msg->_impl_.matrixfile_;
}
const ::Odb::Lib::Protobuf::StandardFontsFile&
FileArchive::_Internal::standardfontsfile(const FileArchive* msg) {
  return *msg->_impl_.standardfontsfile_;
}
void FileArchive::clear_stepsbyname() {
  _impl_.stepsbyname_.Clear();
}
void FileArchive::clear_miscinfofile() {
  if (_impl_.miscinfofile_ != nullptr) _impl_.miscinfofile_->Clear();
  _impl_._has_bits_[0] &= ~0x00000001u;
}
void FileArchive::clear_matrixfile() {
  if (_impl_.matrixfile_ != nullptr) _impl_.matrixfile_->Clear();
  _impl_._has_bits_[0] &= ~0x00000002u;
}
void FileArchive::clear_standardfontsfile() {
  if (_impl_.standardfontsfile_ != nullptr) _impl_.standardfontsfile_->Clear();
  _impl_._has_bits_[0] &= ~0x00000004u;
}
FileArchive::FileArchive(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  if (arena != nullptr && !is_message_owned) {
    arena->OwnCustomDestructor(this, &FileArchive::ArenaDtor);
  }
  // @@protoc_insertion_point(arena_constructor:Odb.Lib.Protobuf.FileArchive)
}
FileArchive::FileArchive(const FileArchive& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  FileArchive* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_._has_bits_){from._impl_._has_bits_}
    , /*decltype(_impl_._cached_size_)*/{}
    , /*decltype(_impl_.stepsbyname_)*/{}
    , decltype(_impl_.miscinfofile_){nullptr}
    , decltype(_impl_.matrixfile_){nullptr}
    , decltype(_impl_.standardfontsfile_){nullptr}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  _this->_impl_.stepsbyname_.MergeFrom(from._impl_.stepsbyname_);
  if (from._internal_has_miscinfofile()) {
    _this->_impl_.miscinfofile_ = new ::Odb::Lib::Protobuf::MiscInfoFile(*from._impl_.miscinfofile_);
  }
  if (from._internal_has_matrixfile()) {
    _this->_impl_.matrixfile_ = new ::Odb::Lib::Protobuf::MatrixFile(*from._impl_.matrixfile_);
  }
  if (from._internal_has_standardfontsfile()) {
    _this->_impl_.standardfontsfile_ = new ::Odb::Lib::Protobuf::StandardFontsFile(*from._impl_.standardfontsfile_);
  }
  // @@protoc_insertion_point(copy_constructor:Odb.Lib.Protobuf.FileArchive)
}

inline void FileArchive::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_._has_bits_){}
    , /*decltype(_impl_._cached_size_)*/{}
    , /*decltype(_impl_.stepsbyname_)*/{::_pbi::ArenaInitialized(), arena}
    , decltype(_impl_.miscinfofile_){nullptr}
    , decltype(_impl_.matrixfile_){nullptr}
    , decltype(_impl_.standardfontsfile_){nullptr}
  };
}

FileArchive::~FileArchive() {
  // @@protoc_insertion_point(destructor:Odb.Lib.Protobuf.FileArchive)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    ArenaDtor(this);
    return;
  }
  SharedDtor();
}

inline void FileArchive::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  _impl_.stepsbyname_.Destruct();
  _impl_.stepsbyname_.~MapField();
  if (this != internal_default_instance()) delete _impl_.miscinfofile_;
  if (this != internal_default_instance()) delete _impl_.matrixfile_;
  if (this != internal_default_instance()) delete _impl_.standardfontsfile_;
}

void FileArchive::ArenaDtor(void* object) {
  FileArchive* _this = reinterpret_cast< FileArchive* >(object);
  _this->_impl_.stepsbyname_.Destruct();
}
void FileArchive::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void FileArchive::Clear() {
// @@protoc_insertion_point(message_clear_start:Odb.Lib.Protobuf.FileArchive)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.stepsbyname_.Clear();
  cached_has_bits = _impl_._has_bits_[0];
  if (cached_has_bits & 0x00000007u) {
    if (cached_has_bits & 0x00000001u) {
      GOOGLE_DCHECK(_impl_.miscinfofile_ != nullptr);
      _impl_.miscinfofile_->Clear();
    }
    if (cached_has_bits & 0x00000002u) {
      GOOGLE_DCHECK(_impl_.matrixfile_ != nullptr);
      _impl_.matrixfile_->Clear();
    }
    if (cached_has_bits & 0x00000004u) {
      GOOGLE_DCHECK(_impl_.standardfontsfile_ != nullptr);
      _impl_.standardfontsfile_->Clear();
    }
  }
  _impl_._has_bits_.Clear();
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* FileArchive::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  _Internal::HasBits has_bits{};
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // map<string, .Odb.Lib.Protobuf.StepDirectory> stepsByName = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          ptr -= 1;
          do {
            ptr += 1;
            ptr = ctx->ParseMessage(&_impl_.stepsbyname_, ptr);
            CHK_(ptr);
            if (!ctx->DataAvailable(ptr)) break;
          } while (::PROTOBUF_NAMESPACE_ID::internal::ExpectTag<10>(ptr));
        } else
          goto handle_unusual;
        continue;
      // optional .Odb.Lib.Protobuf.MiscInfoFile miscInfoFile = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 18)) {
          ptr = ctx->ParseMessage(_internal_mutable_miscinfofile(), ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // optional .Odb.Lib.Protobuf.MatrixFile matrixFile = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 26)) {
          ptr = ctx->ParseMessage(_internal_mutable_matrixfile(), ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      // optional .Odb.Lib.Protobuf.StandardFontsFile standardFontsFile = 4;
      case 4:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 34)) {
          ptr = ctx->ParseMessage(_internal_mutable_standardfontsfile(), ptr);
          CHK_(ptr);
        } else
          goto handle_unusual;
        continue;
      default:
        goto handle_unusual;
    }  // switch
  handle_unusual:
    if ((tag == 0) || ((tag & 7) == 4)) {
      CHK_(ptr);
      ctx->SetLastTag(tag);
      goto message_done;
    }
    ptr = UnknownFieldParse(
        tag,
        _internal_metadata_.mutable_unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(),
        ptr, ctx);
    CHK_(ptr != nullptr);
  }  // while
message_done:
  _impl_._has_bits_.Or(has_bits);
  return ptr;
failure:
  ptr = nullptr;
  goto message_done;
#undef CHK_
}

uint8_t* FileArchive::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:Odb.Lib.Protobuf.FileArchive)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // map<string, .Odb.Lib.Protobuf.StepDirectory> stepsByName = 1;
  if (!this->_internal_stepsbyname().empty()) {
    using MapType = ::_pb::Map<std::string, ::Odb::Lib::Protobuf::StepDirectory>;
    using WireHelper = FileArchive_StepsByNameEntry_DoNotUse::Funcs;
    const auto& map_field = this->_internal_stepsbyname();
    auto check_utf8 = [](const MapType::value_type& entry) {
      (void)entry;
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
        entry.first.data(), static_cast<int>(entry.first.length()),
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
        "Odb.Lib.Protobuf.FileArchive.StepsByNameEntry.key");
    };

    if (stream->IsSerializationDeterministic() && map_field.size() > 1) {
      for (const auto& entry : ::_pbi::MapSorterPtr<MapType>(map_field)) {
        target = WireHelper::InternalSerialize(1, entry.first, entry.second, target, stream);
        check_utf8(entry);
      }
    } else {
      for (const auto& entry : map_field) {
        target = WireHelper::InternalSerialize(1, entry.first, entry.second, target, stream);
        check_utf8(entry);
      }
    }
  }

  // optional .Odb.Lib.Protobuf.MiscInfoFile miscInfoFile = 2;
  if (_internal_has_miscinfofile()) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(2, _Internal::miscinfofile(this),
        _Internal::miscinfofile(this).GetCachedSize(), target, stream);
  }

  // optional .Odb.Lib.Protobuf.MatrixFile matrixFile = 3;
  if (_internal_has_matrixfile()) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(3, _Internal::matrixfile(this),
        _Internal::matrixfile(this).GetCachedSize(), target, stream);
  }

  // optional .Odb.Lib.Protobuf.StandardFontsFile standardFontsFile = 4;
  if (_internal_has_standardfontsfile()) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::
      InternalWriteMessage(4, _Internal::standardfontsfile(this),
        _Internal::standardfontsfile(this).GetCachedSize(), target, stream);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:Odb.Lib.Protobuf.FileArchive)
  return target;
}

size_t FileArchive::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:Odb.Lib.Protobuf.FileArchive)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // map<string, .Odb.Lib.Protobuf.StepDirectory> stepsByName = 1;
  total_size += 1 *
      ::PROTOBUF_NAMESPACE_ID::internal::FromIntSize(this->_internal_stepsbyname_size());
  for (::PROTOBUF_NAMESPACE_ID::Map< std::string, ::Odb::Lib::Protobuf::StepDirectory >::const_iterator
      it = this->_internal_stepsbyname().begin();
      it != this->_internal_stepsbyname().end(); ++it) {
    total_size += FileArchive_StepsByNameEntry_DoNotUse::Funcs::ByteSizeLong(it->first, it->second);
  }

  cached_has_bits = _impl_._has_bits_[0];
  if (cached_has_bits & 0x00000007u) {
    // optional .Odb.Lib.Protobuf.MiscInfoFile miscInfoFile = 2;
    if (cached_has_bits & 0x00000001u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(
          *_impl_.miscinfofile_);
    }

    // optional .Odb.Lib.Protobuf.MatrixFile matrixFile = 3;
    if (cached_has_bits & 0x00000002u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(
          *_impl_.matrixfile_);
    }

    // optional .Odb.Lib.Protobuf.StandardFontsFile standardFontsFile = 4;
    if (cached_has_bits & 0x00000004u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::MessageSize(
          *_impl_.standardfontsfile_);
    }

  }
  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData FileArchive::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    FileArchive::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*FileArchive::GetClassData() const { return &_class_data_; }


void FileArchive::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<FileArchive*>(&to_msg);
  auto& from = static_cast<const FileArchive&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:Odb.Lib.Protobuf.FileArchive)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  _this->_impl_.stepsbyname_.MergeFrom(from._impl_.stepsbyname_);
  cached_has_bits = from._impl_._has_bits_[0];
  if (cached_has_bits & 0x00000007u) {
    if (cached_has_bits & 0x00000001u) {
      _this->_internal_mutable_miscinfofile()->::Odb::Lib::Protobuf::MiscInfoFile::MergeFrom(
          from._internal_miscinfofile());
    }
    if (cached_has_bits & 0x00000002u) {
      _this->_internal_mutable_matrixfile()->::Odb::Lib::Protobuf::MatrixFile::MergeFrom(
          from._internal_matrixfile());
    }
    if (cached_has_bits & 0x00000004u) {
      _this->_internal_mutable_standardfontsfile()->::Odb::Lib::Protobuf::StandardFontsFile::MergeFrom(
          from._internal_standardfontsfile());
    }
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void FileArchive::CopyFrom(const FileArchive& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:Odb.Lib.Protobuf.FileArchive)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool FileArchive::IsInitialized() const {
  return true;
}

void FileArchive::InternalSwap(FileArchive* other) {
  using std::swap;
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  swap(_impl_._has_bits_[0], other->_impl_._has_bits_[0]);
  _impl_.stepsbyname_.InternalSwap(&other->_impl_.stepsbyname_);
  ::PROTOBUF_NAMESPACE_ID::internal::memswap<
      PROTOBUF_FIELD_OFFSET(FileArchive, _impl_.standardfontsfile_)
      + sizeof(FileArchive::_impl_.standardfontsfile_)
      - PROTOBUF_FIELD_OFFSET(FileArchive, _impl_.miscinfofile_)>(
          reinterpret_cast<char*>(&_impl_.miscinfofile_),
          reinterpret_cast<char*>(&other->_impl_.miscinfofile_));
}

::PROTOBUF_NAMESPACE_ID::Metadata FileArchive::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_filearchive_2eproto_getter, &descriptor_table_filearchive_2eproto_once,
      file_level_metadata_filearchive_2eproto[1]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::Odb::Lib::Protobuf::FileArchive_StepsByNameEntry_DoNotUse*
Arena::CreateMaybeMessage< ::Odb::Lib::Protobuf::FileArchive_StepsByNameEntry_DoNotUse >(Arena* arena) {
  return Arena::CreateMessageInternal< ::Odb::Lib::Protobuf::FileArchive_StepsByNameEntry_DoNotUse >(arena);
}
template<> PROTOBUF_NOINLINE ::Odb::Lib::Protobuf::FileArchive*
Arena::CreateMaybeMessage< ::Odb::Lib::Protobuf::FileArchive >(Arena* arena) {
  return Arena::CreateMessageInternal< ::Odb::Lib::Protobuf::FileArchive >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
