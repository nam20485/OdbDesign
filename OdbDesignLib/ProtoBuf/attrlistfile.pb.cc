// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: attrlistfile.proto

#include "attrlistfile.pb.h"

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
PROTOBUF_CONSTEXPR AttrListFile_AttributesByNameEntry_DoNotUse::AttrListFile_AttributesByNameEntry_DoNotUse(
    ::_pbi::ConstantInitialized) {}
struct AttrListFile_AttributesByNameEntry_DoNotUseDefaultTypeInternal {
  PROTOBUF_CONSTEXPR AttrListFile_AttributesByNameEntry_DoNotUseDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~AttrListFile_AttributesByNameEntry_DoNotUseDefaultTypeInternal() {}
  union {
    AttrListFile_AttributesByNameEntry_DoNotUse _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 AttrListFile_AttributesByNameEntry_DoNotUseDefaultTypeInternal _AttrListFile_AttributesByNameEntry_DoNotUse_default_instance_;
PROTOBUF_CONSTEXPR AttrListFile::AttrListFile(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_._has_bits_)*/{}
  , /*decltype(_impl_._cached_size_)*/{}
  , /*decltype(_impl_.attributesbyname_)*/{::_pbi::ConstantInitialized()}
  , /*decltype(_impl_.directory_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}
  , /*decltype(_impl_.path_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}
  , /*decltype(_impl_.units_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}} {}
struct AttrListFileDefaultTypeInternal {
  PROTOBUF_CONSTEXPR AttrListFileDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~AttrListFileDefaultTypeInternal() {}
  union {
    AttrListFile _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 AttrListFileDefaultTypeInternal _AttrListFile_default_instance_;
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
static ::_pb::Metadata file_level_metadata_attrlistfile_2eproto[2];
static constexpr ::_pb::EnumDescriptor const** file_level_enum_descriptors_attrlistfile_2eproto = nullptr;
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_attrlistfile_2eproto = nullptr;

const uint32_t TableStruct_attrlistfile_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse, _has_bits_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse, key_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse, value_),
  0,
  1,
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile, _impl_._has_bits_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile, _impl_.directory_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile, _impl_.path_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile, _impl_.units_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile, _impl_.attributesbyname_),
  0,
  1,
  2,
  ~0u,
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, 8, -1, sizeof(::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse)},
  { 10, 20, -1, sizeof(::Odb::Lib::Protobuf::AttrListFile)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::Odb::Lib::Protobuf::_AttrListFile_AttributesByNameEntry_DoNotUse_default_instance_._instance,
  &::Odb::Lib::Protobuf::_AttrListFile_default_instance_._instance,
};

const char descriptor_table_protodef_attrlistfile_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\022attrlistfile.proto\022\020Odb.Lib.Protobuf\"\367"
  "\001\n\014AttrListFile\022\026\n\tdirectory\030\001 \001(\tH\000\210\001\001\022"
  "\021\n\004path\030\002 \001(\tH\001\210\001\001\022\022\n\005units\030\003 \001(\tH\002\210\001\001\022N"
  "\n\020attributesByName\030\004 \003(\01324.Odb.Lib.Proto"
  "buf.AttrListFile.AttributesByNameEntry\0327"
  "\n\025AttributesByNameEntry\022\013\n\003key\030\001 \001(\t\022\r\n\005"
  "value\030\002 \001(\t:\0028\001B\014\n\n_directoryB\007\n\005_pathB\010"
  "\n\006_unitsb\006proto3"
  ;
static ::_pbi::once_flag descriptor_table_attrlistfile_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_attrlistfile_2eproto = {
    false, false, 296, descriptor_table_protodef_attrlistfile_2eproto,
    "attrlistfile.proto",
    &descriptor_table_attrlistfile_2eproto_once, nullptr, 0, 2,
    schemas, file_default_instances, TableStruct_attrlistfile_2eproto::offsets,
    file_level_metadata_attrlistfile_2eproto, file_level_enum_descriptors_attrlistfile_2eproto,
    file_level_service_descriptors_attrlistfile_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_attrlistfile_2eproto_getter() {
  return &descriptor_table_attrlistfile_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_attrlistfile_2eproto(&descriptor_table_attrlistfile_2eproto);
namespace Odb {
namespace Lib {
namespace Protobuf {

// ===================================================================

AttrListFile_AttributesByNameEntry_DoNotUse::AttrListFile_AttributesByNameEntry_DoNotUse() {}
AttrListFile_AttributesByNameEntry_DoNotUse::AttrListFile_AttributesByNameEntry_DoNotUse(::PROTOBUF_NAMESPACE_ID::Arena* arena)
    : SuperType(arena) {}
void AttrListFile_AttributesByNameEntry_DoNotUse::MergeFrom(const AttrListFile_AttributesByNameEntry_DoNotUse& other) {
  MergeFromInternal(other);
}
::PROTOBUF_NAMESPACE_ID::Metadata AttrListFile_AttributesByNameEntry_DoNotUse::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_attrlistfile_2eproto_getter, &descriptor_table_attrlistfile_2eproto_once,
      file_level_metadata_attrlistfile_2eproto[0]);
}

// ===================================================================

class AttrListFile::_Internal {
 public:
  using HasBits = decltype(std::declval<AttrListFile>()._impl_._has_bits_);
  static void set_has_directory(HasBits* has_bits) {
    (*has_bits)[0] |= 1u;
  }
  static void set_has_path(HasBits* has_bits) {
    (*has_bits)[0] |= 2u;
  }
  static void set_has_units(HasBits* has_bits) {
    (*has_bits)[0] |= 4u;
  }
};

AttrListFile::AttrListFile(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  if (arena != nullptr && !is_message_owned) {
    arena->OwnCustomDestructor(this, &AttrListFile::ArenaDtor);
  }
  // @@protoc_insertion_point(arena_constructor:Odb.Lib.Protobuf.AttrListFile)
}
AttrListFile::AttrListFile(const AttrListFile& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  AttrListFile* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_._has_bits_){from._impl_._has_bits_}
    , /*decltype(_impl_._cached_size_)*/{}
    , /*decltype(_impl_.attributesbyname_)*/{}
    , decltype(_impl_.directory_){}
    , decltype(_impl_.path_){}
    , decltype(_impl_.units_){}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  _this->_impl_.attributesbyname_.MergeFrom(from._impl_.attributesbyname_);
  _impl_.directory_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.directory_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (from._internal_has_directory()) {
    _this->_impl_.directory_.Set(from._internal_directory(), 
      _this->GetArenaForAllocation());
  }
  _impl_.path_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.path_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (from._internal_has_path()) {
    _this->_impl_.path_.Set(from._internal_path(), 
      _this->GetArenaForAllocation());
  }
  _impl_.units_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.units_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (from._internal_has_units()) {
    _this->_impl_.units_.Set(from._internal_units(), 
      _this->GetArenaForAllocation());
  }
  // @@protoc_insertion_point(copy_constructor:Odb.Lib.Protobuf.AttrListFile)
}

inline void AttrListFile::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_._has_bits_){}
    , /*decltype(_impl_._cached_size_)*/{}
    , /*decltype(_impl_.attributesbyname_)*/{::_pbi::ArenaInitialized(), arena}
    , decltype(_impl_.directory_){}
    , decltype(_impl_.path_){}
    , decltype(_impl_.units_){}
  };
  _impl_.directory_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.directory_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  _impl_.path_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.path_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  _impl_.units_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.units_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
}

AttrListFile::~AttrListFile() {
  // @@protoc_insertion_point(destructor:Odb.Lib.Protobuf.AttrListFile)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    ArenaDtor(this);
    return;
  }
  SharedDtor();
}

inline void AttrListFile::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  _impl_.attributesbyname_.Destruct();
  _impl_.attributesbyname_.~MapField();
  _impl_.directory_.Destroy();
  _impl_.path_.Destroy();
  _impl_.units_.Destroy();
}

void AttrListFile::ArenaDtor(void* object) {
  AttrListFile* _this = reinterpret_cast< AttrListFile* >(object);
  _this->_impl_.attributesbyname_.Destruct();
}
void AttrListFile::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void AttrListFile::Clear() {
// @@protoc_insertion_point(message_clear_start:Odb.Lib.Protobuf.AttrListFile)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  _impl_.attributesbyname_.Clear();
  cached_has_bits = _impl_._has_bits_[0];
  if (cached_has_bits & 0x00000007u) {
    if (cached_has_bits & 0x00000001u) {
      _impl_.directory_.ClearNonDefaultToEmpty();
    }
    if (cached_has_bits & 0x00000002u) {
      _impl_.path_.ClearNonDefaultToEmpty();
    }
    if (cached_has_bits & 0x00000004u) {
      _impl_.units_.ClearNonDefaultToEmpty();
    }
  }
  _impl_._has_bits_.Clear();
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* AttrListFile::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  _Internal::HasBits has_bits{};
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // optional string directory = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          auto str = _internal_mutable_directory();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
          CHK_(::_pbi::VerifyUTF8(str, "Odb.Lib.Protobuf.AttrListFile.directory"));
        } else
          goto handle_unusual;
        continue;
      // optional string path = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 18)) {
          auto str = _internal_mutable_path();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
          CHK_(::_pbi::VerifyUTF8(str, "Odb.Lib.Protobuf.AttrListFile.path"));
        } else
          goto handle_unusual;
        continue;
      // optional string units = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 26)) {
          auto str = _internal_mutable_units();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
          CHK_(::_pbi::VerifyUTF8(str, "Odb.Lib.Protobuf.AttrListFile.units"));
        } else
          goto handle_unusual;
        continue;
      // map<string, string> attributesByName = 4;
      case 4:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 34)) {
          ptr -= 1;
          do {
            ptr += 1;
            ptr = ctx->ParseMessage(&_impl_.attributesbyname_, ptr);
            CHK_(ptr);
            if (!ctx->DataAvailable(ptr)) break;
          } while (::PROTOBUF_NAMESPACE_ID::internal::ExpectTag<34>(ptr));
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

uint8_t* AttrListFile::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:Odb.Lib.Protobuf.AttrListFile)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // optional string directory = 1;
  if (_internal_has_directory()) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_directory().data(), static_cast<int>(this->_internal_directory().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "Odb.Lib.Protobuf.AttrListFile.directory");
    target = stream->WriteStringMaybeAliased(
        1, this->_internal_directory(), target);
  }

  // optional string path = 2;
  if (_internal_has_path()) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_path().data(), static_cast<int>(this->_internal_path().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "Odb.Lib.Protobuf.AttrListFile.path");
    target = stream->WriteStringMaybeAliased(
        2, this->_internal_path(), target);
  }

  // optional string units = 3;
  if (_internal_has_units()) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_units().data(), static_cast<int>(this->_internal_units().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "Odb.Lib.Protobuf.AttrListFile.units");
    target = stream->WriteStringMaybeAliased(
        3, this->_internal_units(), target);
  }

  // map<string, string> attributesByName = 4;
  if (!this->_internal_attributesbyname().empty()) {
    using MapType = ::_pb::Map<std::string, std::string>;
    using WireHelper = AttrListFile_AttributesByNameEntry_DoNotUse::Funcs;
    const auto& map_field = this->_internal_attributesbyname();
    auto check_utf8 = [](const MapType::value_type& entry) {
      (void)entry;
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
        entry.first.data(), static_cast<int>(entry.first.length()),
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
        "Odb.Lib.Protobuf.AttrListFile.AttributesByNameEntry.key");
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
        entry.second.data(), static_cast<int>(entry.second.length()),
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
        "Odb.Lib.Protobuf.AttrListFile.AttributesByNameEntry.value");
    };

    if (stream->IsSerializationDeterministic() && map_field.size() > 1) {
      for (const auto& entry : ::_pbi::MapSorterPtr<MapType>(map_field)) {
        target = WireHelper::InternalSerialize(4, entry.first, entry.second, target, stream);
        check_utf8(entry);
      }
    } else {
      for (const auto& entry : map_field) {
        target = WireHelper::InternalSerialize(4, entry.first, entry.second, target, stream);
        check_utf8(entry);
      }
    }
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:Odb.Lib.Protobuf.AttrListFile)
  return target;
}

size_t AttrListFile::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:Odb.Lib.Protobuf.AttrListFile)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // map<string, string> attributesByName = 4;
  total_size += 1 *
      ::PROTOBUF_NAMESPACE_ID::internal::FromIntSize(this->_internal_attributesbyname_size());
  for (::PROTOBUF_NAMESPACE_ID::Map< std::string, std::string >::const_iterator
      it = this->_internal_attributesbyname().begin();
      it != this->_internal_attributesbyname().end(); ++it) {
    total_size += AttrListFile_AttributesByNameEntry_DoNotUse::Funcs::ByteSizeLong(it->first, it->second);
  }

  cached_has_bits = _impl_._has_bits_[0];
  if (cached_has_bits & 0x00000007u) {
    // optional string directory = 1;
    if (cached_has_bits & 0x00000001u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
          this->_internal_directory());
    }

    // optional string path = 2;
    if (cached_has_bits & 0x00000002u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
          this->_internal_path());
    }

    // optional string units = 3;
    if (cached_has_bits & 0x00000004u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
          this->_internal_units());
    }

  }
  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData AttrListFile::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    AttrListFile::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*AttrListFile::GetClassData() const { return &_class_data_; }


void AttrListFile::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<AttrListFile*>(&to_msg);
  auto& from = static_cast<const AttrListFile&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:Odb.Lib.Protobuf.AttrListFile)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  _this->_impl_.attributesbyname_.MergeFrom(from._impl_.attributesbyname_);
  cached_has_bits = from._impl_._has_bits_[0];
  if (cached_has_bits & 0x00000007u) {
    if (cached_has_bits & 0x00000001u) {
      _this->_internal_set_directory(from._internal_directory());
    }
    if (cached_has_bits & 0x00000002u) {
      _this->_internal_set_path(from._internal_path());
    }
    if (cached_has_bits & 0x00000004u) {
      _this->_internal_set_units(from._internal_units());
    }
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void AttrListFile::CopyFrom(const AttrListFile& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:Odb.Lib.Protobuf.AttrListFile)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool AttrListFile::IsInitialized() const {
  return true;
}

void AttrListFile::InternalSwap(AttrListFile* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  swap(_impl_._has_bits_[0], other->_impl_._has_bits_[0]);
  _impl_.attributesbyname_.InternalSwap(&other->_impl_.attributesbyname_);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.directory_, lhs_arena,
      &other->_impl_.directory_, rhs_arena
  );
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.path_, lhs_arena,
      &other->_impl_.path_, rhs_arena
  );
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.units_, lhs_arena,
      &other->_impl_.units_, rhs_arena
  );
}

::PROTOBUF_NAMESPACE_ID::Metadata AttrListFile::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_attrlistfile_2eproto_getter, &descriptor_table_attrlistfile_2eproto_once,
      file_level_metadata_attrlistfile_2eproto[1]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse*
Arena::CreateMaybeMessage< ::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse >(Arena* arena) {
  return Arena::CreateMessageInternal< ::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse >(arena);
}
template<> PROTOBUF_NOINLINE ::Odb::Lib::Protobuf::AttrListFile*
Arena::CreateMaybeMessage< ::Odb::Lib::Protobuf::AttrListFile >(Arena* arena) {
  return Arena::CreateMessageInternal< ::Odb::Lib::Protobuf::AttrListFile >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>