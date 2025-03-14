// Generated by the protocol buffer compiler.  DO NOT EDIT!
// NO CHECKED-IN PROTOBUF GENCODE
// source: attrlistfile.proto
// Protobuf C++ Version: 5.29.2

#include "attrlistfile.pb.h"

#include <algorithm>
#include <type_traits>
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/generated_message_tctable_impl.h"
#include "google/protobuf/extension_set.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/wire_format_lite.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/reflection_ops.h"
#include "google/protobuf/wire_format.h"
// @@protoc_insertion_point(includes)

// Must be included last.
#include "google/protobuf/port_def.inc"
PROTOBUF_PRAGMA_INIT_SEG
namespace _pb = ::google::protobuf;
namespace _pbi = ::google::protobuf::internal;
namespace _fl = ::google::protobuf::internal::field_layout;
namespace Odb {
namespace Lib {
namespace Protobuf {
              template <typename>
PROTOBUF_CONSTEXPR AttrListFile_AttributesByNameEntry_DoNotUse::AttrListFile_AttributesByNameEntry_DoNotUse(::_pbi::ConstantInitialized)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : AttrListFile_AttributesByNameEntry_DoNotUse::MapEntry(_class_data_.base()){}
#else   // PROTOBUF_CUSTOM_VTABLE
    : AttrListFile_AttributesByNameEntry_DoNotUse::MapEntry() {
}
#endif  // PROTOBUF_CUSTOM_VTABLE
struct AttrListFile_AttributesByNameEntry_DoNotUseDefaultTypeInternal {
  PROTOBUF_CONSTEXPR AttrListFile_AttributesByNameEntry_DoNotUseDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~AttrListFile_AttributesByNameEntry_DoNotUseDefaultTypeInternal() {}
  union {
    AttrListFile_AttributesByNameEntry_DoNotUse _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ODBDESIGN_EXPORT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 AttrListFile_AttributesByNameEntry_DoNotUseDefaultTypeInternal _AttrListFile_AttributesByNameEntry_DoNotUse_default_instance_;

inline constexpr AttrListFile::Impl_::Impl_(
    ::_pbi::ConstantInitialized) noexcept
      : _cached_size_{0},
        attributesbyname_{},
        directory_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        path_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        units_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()) {}

template <typename>
PROTOBUF_CONSTEXPR AttrListFile::AttrListFile(::_pbi::ConstantInitialized)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(_class_data_.base()),
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(),
#endif  // PROTOBUF_CUSTOM_VTABLE
      _impl_(::_pbi::ConstantInitialized()) {
}
struct AttrListFileDefaultTypeInternal {
  PROTOBUF_CONSTEXPR AttrListFileDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~AttrListFileDefaultTypeInternal() {}
  union {
    AttrListFile _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ODBDESIGN_EXPORT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 AttrListFileDefaultTypeInternal _AttrListFile_default_instance_;
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
static constexpr const ::_pb::EnumDescriptor**
    file_level_enum_descriptors_attrlistfile_2eproto = nullptr;
static constexpr const ::_pb::ServiceDescriptor**
    file_level_service_descriptors_attrlistfile_2eproto = nullptr;
const ::uint32_t
    TableStruct_attrlistfile_2eproto::offsets[] ABSL_ATTRIBUTE_SECTION_VARIABLE(
        protodesc_cold) = {
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse, _impl_._has_bits_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse, _internal_metadata_),
        ~0u,  // no _extensions_
        ~0u,  // no _oneof_case_
        ~0u,  // no _weak_field_map_
        ~0u,  // no _inlined_string_donated_
        ~0u,  // no _split_
        ~0u,  // no sizeof(Split)
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse, _impl_.key_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse, _impl_.value_),
        0,
        1,
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile, _impl_._has_bits_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile, _internal_metadata_),
        ~0u,  // no _extensions_
        ~0u,  // no _oneof_case_
        ~0u,  // no _weak_field_map_
        ~0u,  // no _inlined_string_donated_
        ~0u,  // no _split_
        ~0u,  // no sizeof(Split)
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile, _impl_.directory_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile, _impl_.path_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile, _impl_.units_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::AttrListFile, _impl_.attributesbyname_),
        0,
        1,
        2,
        ~0u,
};

static const ::_pbi::MigrationSchema
    schemas[] ABSL_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
        {0, 10, -1, sizeof(::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse)},
        {12, 24, -1, sizeof(::Odb::Lib::Protobuf::AttrListFile)},
};
static const ::_pb::Message* const file_default_instances[] = {
    &::Odb::Lib::Protobuf::_AttrListFile_AttributesByNameEntry_DoNotUse_default_instance_._instance,
    &::Odb::Lib::Protobuf::_AttrListFile_default_instance_._instance,
};
const char descriptor_table_protodef_attrlistfile_2eproto[] ABSL_ATTRIBUTE_SECTION_VARIABLE(
    protodesc_cold) = {
    "\n\022attrlistfile.proto\022\020Odb.Lib.Protobuf\"\367"
    "\001\n\014AttrListFile\022\026\n\tdirectory\030\001 \001(\tH\000\210\001\001\022"
    "\021\n\004path\030\002 \001(\tH\001\210\001\001\022\022\n\005units\030\003 \001(\tH\002\210\001\001\022N"
    "\n\020attributesByName\030\004 \003(\01324.Odb.Lib.Proto"
    "buf.AttrListFile.AttributesByNameEntry\0327"
    "\n\025AttributesByNameEntry\022\013\n\003key\030\001 \001(\t\022\r\n\005"
    "value\030\002 \001(\t:\0028\001B\014\n\n_directoryB\007\n\005_pathB\010"
    "\n\006_unitsb\006proto3"
};
static ::absl::once_flag descriptor_table_attrlistfile_2eproto_once;
PROTOBUF_CONSTINIT const ::_pbi::DescriptorTable descriptor_table_attrlistfile_2eproto = {
    false,
    false,
    296,
    descriptor_table_protodef_attrlistfile_2eproto,
    "attrlistfile.proto",
    &descriptor_table_attrlistfile_2eproto_once,
    nullptr,
    0,
    2,
    schemas,
    file_default_instances,
    TableStruct_attrlistfile_2eproto::offsets,
    file_level_enum_descriptors_attrlistfile_2eproto,
    file_level_service_descriptors_attrlistfile_2eproto,
};
namespace Odb {
namespace Lib {
namespace Protobuf {
// ===================================================================

#if defined(PROTOBUF_CUSTOM_VTABLE)
              AttrListFile_AttributesByNameEntry_DoNotUse::AttrListFile_AttributesByNameEntry_DoNotUse() : SuperType(_class_data_.base()) {}
              AttrListFile_AttributesByNameEntry_DoNotUse::AttrListFile_AttributesByNameEntry_DoNotUse(::google::protobuf::Arena* arena)
                  : SuperType(arena, _class_data_.base()) {}
#else   // PROTOBUF_CUSTOM_VTABLE
              AttrListFile_AttributesByNameEntry_DoNotUse::AttrListFile_AttributesByNameEntry_DoNotUse() : SuperType() {}
              AttrListFile_AttributesByNameEntry_DoNotUse::AttrListFile_AttributesByNameEntry_DoNotUse(::google::protobuf::Arena* arena) : SuperType(arena) {}
#endif  // PROTOBUF_CUSTOM_VTABLE
              inline void* AttrListFile_AttributesByNameEntry_DoNotUse::PlacementNew_(const void*, void* mem,
                                                      ::google::protobuf::Arena* arena) {
                return ::new (mem) AttrListFile_AttributesByNameEntry_DoNotUse(arena);
              }
              constexpr auto AttrListFile_AttributesByNameEntry_DoNotUse::InternalNewImpl_() {
                return ::google::protobuf::internal::MessageCreator::CopyInit(sizeof(AttrListFile_AttributesByNameEntry_DoNotUse),
                                                          alignof(AttrListFile_AttributesByNameEntry_DoNotUse));
              }
              PROTOBUF_CONSTINIT
              PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
              const ::google::protobuf::internal::ClassDataFull AttrListFile_AttributesByNameEntry_DoNotUse::_class_data_ = {
                  ::google::protobuf::internal::ClassData{
                      &_AttrListFile_AttributesByNameEntry_DoNotUse_default_instance_._instance,
                      &_table_.header,
                      nullptr,  // OnDemandRegisterArenaDtor
                      nullptr,  // IsInitialized
                      &AttrListFile_AttributesByNameEntry_DoNotUse::MergeImpl,
                      ::google::protobuf::Message::GetNewImpl<AttrListFile_AttributesByNameEntry_DoNotUse>(),
              #if defined(PROTOBUF_CUSTOM_VTABLE)
                      &AttrListFile_AttributesByNameEntry_DoNotUse::SharedDtor,
                      static_cast<void (::google::protobuf::MessageLite::*)()>(
                          &AttrListFile_AttributesByNameEntry_DoNotUse::ClearImpl),
                          ::google::protobuf::Message::ByteSizeLongImpl, ::google::protobuf::Message::_InternalSerializeImpl
                          ,
              #endif  // PROTOBUF_CUSTOM_VTABLE
                      PROTOBUF_FIELD_OFFSET(AttrListFile_AttributesByNameEntry_DoNotUse, _impl_._cached_size_),
                      false,
                  },
                  &AttrListFile_AttributesByNameEntry_DoNotUse::kDescriptorMethods,
                  &descriptor_table_attrlistfile_2eproto,
                  nullptr,  // tracker
              };
              const ::google::protobuf::internal::ClassData* AttrListFile_AttributesByNameEntry_DoNotUse::GetClassData() const {
                ::google::protobuf::internal::PrefetchToLocalCache(&_class_data_);
                ::google::protobuf::internal::PrefetchToLocalCache(_class_data_.tc_table);
                return _class_data_.base();
              }
PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<1, 2, 0, 68, 2> AttrListFile_AttributesByNameEntry_DoNotUse::_table_ = {
  {
    PROTOBUF_FIELD_OFFSET(AttrListFile_AttributesByNameEntry_DoNotUse, _impl_._has_bits_),
    0, // no _extensions_
    2, 8,  // max_field_number, fast_idx_mask
    offsetof(decltype(_table_), field_lookup_table),
    4294967292,  // skipmap
    offsetof(decltype(_table_), field_entries),
    2,  // num_field_entries
    0,  // num_aux_entries
    offsetof(decltype(_table_), field_names),  // no aux_entries
    _class_data_.base(),
    nullptr,  // post_loop_handler
    ::_pbi::TcParser::DiscardEverythingFallback,  // fallback
    #ifdef PROTOBUF_PREFETCH_PARSE_TABLE
    ::_pbi::TcParser::GetTable<::Odb::Lib::Protobuf::AttrListFile_AttributesByNameEntry_DoNotUse>(),  // to_prefetch
    #endif  // PROTOBUF_PREFETCH_PARSE_TABLE
  }, {{
    // string value = 2;
    {::_pbi::TcParser::FastUS1,
     {18, 63, 0, PROTOBUF_FIELD_OFFSET(AttrListFile_AttributesByNameEntry_DoNotUse, _impl_.value_)}},
    // string key = 1;
    {::_pbi::TcParser::FastUS1,
     {10, 63, 0, PROTOBUF_FIELD_OFFSET(AttrListFile_AttributesByNameEntry_DoNotUse, _impl_.key_)}},
  }}, {{
    65535, 65535
  }}, {{
    // string key = 1;
    {PROTOBUF_FIELD_OFFSET(AttrListFile_AttributesByNameEntry_DoNotUse, _impl_.key_), -1, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kUtf8String | ::_fl::kRepAString)},
    // string value = 2;
    {PROTOBUF_FIELD_OFFSET(AttrListFile_AttributesByNameEntry_DoNotUse, _impl_.value_), -1, 0,
    (0 | ::_fl::kFcSingular | ::_fl::kUtf8String | ::_fl::kRepAString)},
  }},
  // no aux_entries
  {{
    "\63\3\5\0\0\0\0\0"
    "Odb.Lib.Protobuf.AttrListFile.AttributesByNameEntry"
    "key"
    "value"
  }},
};

// ===================================================================

class AttrListFile::_Internal {
 public:
  using HasBits =
      decltype(std::declval<AttrListFile>()._impl_._has_bits_);
  static constexpr ::int32_t kHasBitsOffset =
      8 * PROTOBUF_FIELD_OFFSET(AttrListFile, _impl_._has_bits_);
};

AttrListFile::AttrListFile(::google::protobuf::Arena* arena)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  SharedCtor(arena);
  // @@protoc_insertion_point(arena_constructor:Odb.Lib.Protobuf.AttrListFile)
}
inline PROTOBUF_NDEBUG_INLINE AttrListFile::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility, ::google::protobuf::Arena* arena,
    const Impl_& from, const ::Odb::Lib::Protobuf::AttrListFile& from_msg)
      : _has_bits_{from._has_bits_},
        _cached_size_{0},
        attributesbyname_{visibility, arena, from.attributesbyname_},
        directory_(arena, from.directory_),
        path_(arena, from.path_),
        units_(arena, from.units_) {}

AttrListFile::AttrListFile(
    ::google::protobuf::Arena* arena,
    const AttrListFile& from)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  AttrListFile* const _this = this;
  (void)_this;
  _internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(
      from._internal_metadata_);
  new (&_impl_) Impl_(internal_visibility(), arena, from._impl_, from);

  // @@protoc_insertion_point(copy_constructor:Odb.Lib.Protobuf.AttrListFile)
}
inline PROTOBUF_NDEBUG_INLINE AttrListFile::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility,
    ::google::protobuf::Arena* arena)
      : _cached_size_{0},
        attributesbyname_{visibility, arena},
        directory_(arena),
        path_(arena),
        units_(arena) {}

inline void AttrListFile::SharedCtor(::_pb::Arena* arena) {
  new (&_impl_) Impl_(internal_visibility(), arena);
}
AttrListFile::~AttrListFile() {
  // @@protoc_insertion_point(destructor:Odb.Lib.Protobuf.AttrListFile)
  SharedDtor(*this);
}
inline void AttrListFile::SharedDtor(MessageLite& self) {
  AttrListFile& this_ = static_cast<AttrListFile&>(self);
  this_._internal_metadata_.Delete<::google::protobuf::UnknownFieldSet>();
  ABSL_DCHECK(this_.GetArena() == nullptr);
  this_._impl_.directory_.Destroy();
  this_._impl_.path_.Destroy();
  this_._impl_.units_.Destroy();
  this_._impl_.~Impl_();
}

inline void* AttrListFile::PlacementNew_(const void*, void* mem,
                                        ::google::protobuf::Arena* arena) {
  return ::new (mem) AttrListFile(arena);
}
constexpr auto AttrListFile::InternalNewImpl_() {
  constexpr auto arena_bits = ::google::protobuf::internal::EncodePlacementArenaOffsets({
      PROTOBUF_FIELD_OFFSET(AttrListFile, _impl_.attributesbyname_) +
          decltype(AttrListFile::_impl_.attributesbyname_)::
              InternalGetArenaOffset(
                  ::google::protobuf::Message::internal_visibility()),
      PROTOBUF_FIELD_OFFSET(AttrListFile, _impl_.attributesbyname_) +
          decltype(AttrListFile::_impl_.attributesbyname_)::
              InternalGetArenaOffsetAlt(
                  ::google::protobuf::Message::internal_visibility()),
  });
  if (arena_bits.has_value()) {
    return ::google::protobuf::internal::MessageCreator::CopyInit(
        sizeof(AttrListFile), alignof(AttrListFile), *arena_bits);
  } else {
    return ::google::protobuf::internal::MessageCreator(&AttrListFile::PlacementNew_,
                                 sizeof(AttrListFile),
                                 alignof(AttrListFile));
  }
}
PROTOBUF_CONSTINIT
PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::google::protobuf::internal::ClassDataFull AttrListFile::_class_data_ = {
    ::google::protobuf::internal::ClassData{
        &_AttrListFile_default_instance_._instance,
        &_table_.header,
        nullptr,  // OnDemandRegisterArenaDtor
        nullptr,  // IsInitialized
        &AttrListFile::MergeImpl,
        ::google::protobuf::Message::GetNewImpl<AttrListFile>(),
#if defined(PROTOBUF_CUSTOM_VTABLE)
        &AttrListFile::SharedDtor,
        ::google::protobuf::Message::GetClearImpl<AttrListFile>(), &AttrListFile::ByteSizeLong,
            &AttrListFile::_InternalSerialize,
#endif  // PROTOBUF_CUSTOM_VTABLE
        PROTOBUF_FIELD_OFFSET(AttrListFile, _impl_._cached_size_),
        false,
    },
    &AttrListFile::kDescriptorMethods,
    &descriptor_table_attrlistfile_2eproto,
    nullptr,  // tracker
};
const ::google::protobuf::internal::ClassData* AttrListFile::GetClassData() const {
  ::google::protobuf::internal::PrefetchToLocalCache(&_class_data_);
  ::google::protobuf::internal::PrefetchToLocalCache(_class_data_.tc_table);
  return _class_data_.base();
}
PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<2, 4, 1, 72, 2> AttrListFile::_table_ = {
  {
    PROTOBUF_FIELD_OFFSET(AttrListFile, _impl_._has_bits_),
    0, // no _extensions_
    4, 24,  // max_field_number, fast_idx_mask
    offsetof(decltype(_table_), field_lookup_table),
    4294967280,  // skipmap
    offsetof(decltype(_table_), field_entries),
    4,  // num_field_entries
    1,  // num_aux_entries
    offsetof(decltype(_table_), aux_entries),
    _class_data_.base(),
    nullptr,  // post_loop_handler
    ::_pbi::TcParser::GenericFallback,  // fallback
    #ifdef PROTOBUF_PREFETCH_PARSE_TABLE
    ::_pbi::TcParser::GetTable<::Odb::Lib::Protobuf::AttrListFile>(),  // to_prefetch
    #endif  // PROTOBUF_PREFETCH_PARSE_TABLE
  }, {{
    {::_pbi::TcParser::MiniParse, {}},
    // optional string directory = 1;
    {::_pbi::TcParser::FastUS1,
     {10, 0, 0, PROTOBUF_FIELD_OFFSET(AttrListFile, _impl_.directory_)}},
    // optional string path = 2;
    {::_pbi::TcParser::FastUS1,
     {18, 1, 0, PROTOBUF_FIELD_OFFSET(AttrListFile, _impl_.path_)}},
    // optional string units = 3;
    {::_pbi::TcParser::FastUS1,
     {26, 2, 0, PROTOBUF_FIELD_OFFSET(AttrListFile, _impl_.units_)}},
  }}, {{
    65535, 65535
  }}, {{
    // optional string directory = 1;
    {PROTOBUF_FIELD_OFFSET(AttrListFile, _impl_.directory_), _Internal::kHasBitsOffset + 0, 0,
    (0 | ::_fl::kFcOptional | ::_fl::kUtf8String | ::_fl::kRepAString)},
    // optional string path = 2;
    {PROTOBUF_FIELD_OFFSET(AttrListFile, _impl_.path_), _Internal::kHasBitsOffset + 1, 0,
    (0 | ::_fl::kFcOptional | ::_fl::kUtf8String | ::_fl::kRepAString)},
    // optional string units = 3;
    {PROTOBUF_FIELD_OFFSET(AttrListFile, _impl_.units_), _Internal::kHasBitsOffset + 2, 0,
    (0 | ::_fl::kFcOptional | ::_fl::kUtf8String | ::_fl::kRepAString)},
    // map<string, string> attributesByName = 4;
    {PROTOBUF_FIELD_OFFSET(AttrListFile, _impl_.attributesbyname_), -1, 0,
    (0 | ::_fl::kFcRepeated | ::_fl::kMap)},
  }}, {{
    {::_pbi::TcParser::GetMapAuxInfo<
        decltype(AttrListFile()._impl_.attributesbyname_)>(
        1, 0, 0, 9,
        9)},
  }}, {{
    "\35\11\4\5\20\0\0\0"
    "Odb.Lib.Protobuf.AttrListFile"
    "directory"
    "path"
    "units"
    "attributesByName"
  }},
};

PROTOBUF_NOINLINE void AttrListFile::Clear() {
// @@protoc_insertion_point(message_clear_start:Odb.Lib.Protobuf.AttrListFile)
  ::google::protobuf::internal::TSanWrite(&_impl_);
  ::uint32_t cached_has_bits = 0;
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
  _internal_metadata_.Clear<::google::protobuf::UnknownFieldSet>();
}

#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::uint8_t* AttrListFile::_InternalSerialize(
            const MessageLite& base, ::uint8_t* target,
            ::google::protobuf::io::EpsCopyOutputStream* stream) {
          const AttrListFile& this_ = static_cast<const AttrListFile&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::uint8_t* AttrListFile::_InternalSerialize(
            ::uint8_t* target,
            ::google::protobuf::io::EpsCopyOutputStream* stream) const {
          const AttrListFile& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          // @@protoc_insertion_point(serialize_to_array_start:Odb.Lib.Protobuf.AttrListFile)
          ::uint32_t cached_has_bits = 0;
          (void)cached_has_bits;

          cached_has_bits = this_._impl_._has_bits_[0];
          // optional string directory = 1;
          if (cached_has_bits & 0x00000001u) {
            const std::string& _s = this_._internal_directory();
            ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "Odb.Lib.Protobuf.AttrListFile.directory");
            target = stream->WriteStringMaybeAliased(1, _s, target);
          }

          // optional string path = 2;
          if (cached_has_bits & 0x00000002u) {
            const std::string& _s = this_._internal_path();
            ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "Odb.Lib.Protobuf.AttrListFile.path");
            target = stream->WriteStringMaybeAliased(2, _s, target);
          }

          // optional string units = 3;
          if (cached_has_bits & 0x00000004u) {
            const std::string& _s = this_._internal_units();
            ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "Odb.Lib.Protobuf.AttrListFile.units");
            target = stream->WriteStringMaybeAliased(3, _s, target);
          }

          // map<string, string> attributesByName = 4;
          if (!this_._internal_attributesbyname().empty()) {
            using MapType = ::google::protobuf::Map<std::string, std::string>;
            using WireHelper = _pbi::MapEntryFuncs<std::string, std::string,
                                           _pbi::WireFormatLite::TYPE_STRING,
                                           _pbi::WireFormatLite::TYPE_STRING>;
            const auto& field = this_._internal_attributesbyname();

            if (stream->IsSerializationDeterministic() && field.size() > 1) {
              for (const auto& entry : ::google::protobuf::internal::MapSorterPtr<MapType>(field)) {
                target = WireHelper::InternalSerialize(
                    4, entry.first, entry.second, target, stream);
                ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                    entry.first.data(), static_cast<int>(entry.first.length()),
 ::google::protobuf::internal::WireFormatLite::SERIALIZE, "Odb.Lib.Protobuf.AttrListFile.attributesByName");
                ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                    entry.second.data(), static_cast<int>(entry.second.length()),
 ::google::protobuf::internal::WireFormatLite::SERIALIZE, "Odb.Lib.Protobuf.AttrListFile.attributesByName");
              }
            } else {
              for (const auto& entry : field) {
                target = WireHelper::InternalSerialize(
                    4, entry.first, entry.second, target, stream);
                ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                    entry.first.data(), static_cast<int>(entry.first.length()),
 ::google::protobuf::internal::WireFormatLite::SERIALIZE, "Odb.Lib.Protobuf.AttrListFile.attributesByName");
                ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                    entry.second.data(), static_cast<int>(entry.second.length()),
 ::google::protobuf::internal::WireFormatLite::SERIALIZE, "Odb.Lib.Protobuf.AttrListFile.attributesByName");
              }
            }
          }

          if (PROTOBUF_PREDICT_FALSE(this_._internal_metadata_.have_unknown_fields())) {
            target =
                ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
                    this_._internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance), target, stream);
          }
          // @@protoc_insertion_point(serialize_to_array_end:Odb.Lib.Protobuf.AttrListFile)
          return target;
        }

#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::size_t AttrListFile::ByteSizeLong(const MessageLite& base) {
          const AttrListFile& this_ = static_cast<const AttrListFile&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::size_t AttrListFile::ByteSizeLong() const {
          const AttrListFile& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          // @@protoc_insertion_point(message_byte_size_start:Odb.Lib.Protobuf.AttrListFile)
          ::size_t total_size = 0;

          ::uint32_t cached_has_bits = 0;
          // Prevent compiler warnings about cached_has_bits being unused
          (void)cached_has_bits;

          ::_pbi::Prefetch5LinesFrom7Lines(&this_);
           {
            // map<string, string> attributesByName = 4;
            {
              total_size +=
                  1 * ::google::protobuf::internal::FromIntSize(this_._internal_attributesbyname_size());
              for (const auto& entry : this_._internal_attributesbyname()) {
                total_size += _pbi::MapEntryFuncs<std::string, std::string,
                                               _pbi::WireFormatLite::TYPE_STRING,
                                               _pbi::WireFormatLite::TYPE_STRING>::ByteSizeLong(entry.first, entry.second);
              }
            }
          }
          cached_has_bits = this_._impl_._has_bits_[0];
          if (cached_has_bits & 0x00000007u) {
            // optional string directory = 1;
            if (cached_has_bits & 0x00000001u) {
              total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                              this_._internal_directory());
            }
            // optional string path = 2;
            if (cached_has_bits & 0x00000002u) {
              total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                              this_._internal_path());
            }
            // optional string units = 3;
            if (cached_has_bits & 0x00000004u) {
              total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                              this_._internal_units());
            }
          }
          return this_.MaybeComputeUnknownFieldsSize(total_size,
                                                     &this_._impl_._cached_size_);
        }

void AttrListFile::MergeImpl(::google::protobuf::MessageLite& to_msg, const ::google::protobuf::MessageLite& from_msg) {
  auto* const _this = static_cast<AttrListFile*>(&to_msg);
  auto& from = static_cast<const AttrListFile&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:Odb.Lib.Protobuf.AttrListFile)
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
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
  _this->_impl_._has_bits_[0] |= cached_has_bits;
  _this->_internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(from._internal_metadata_);
}

void AttrListFile::CopyFrom(const AttrListFile& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:Odb.Lib.Protobuf.AttrListFile)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}


void AttrListFile::InternalSwap(AttrListFile* PROTOBUF_RESTRICT other) {
  using std::swap;
  auto* arena = GetArena();
  ABSL_DCHECK_EQ(arena, other->GetArena());
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  swap(_impl_._has_bits_[0], other->_impl_._has_bits_[0]);
  _impl_.attributesbyname_.InternalSwap(&other->_impl_.attributesbyname_);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.directory_, &other->_impl_.directory_, arena);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.path_, &other->_impl_.path_, arena);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.units_, &other->_impl_.units_, arena);
}

::google::protobuf::Metadata AttrListFile::GetMetadata() const {
  return ::google::protobuf::Message::GetMetadataImpl(GetClassData()->full());
}
// @@protoc_insertion_point(namespace_scope)
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
namespace google {
namespace protobuf {
}  // namespace protobuf
}  // namespace google
// @@protoc_insertion_point(global_scope)
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::std::false_type
    _static_init2_ PROTOBUF_UNUSED =
        (::_pbi::AddDescriptors(&descriptor_table_attrlistfile_2eproto),
         ::std::false_type{});
#include "google/protobuf/port_undef.inc"
