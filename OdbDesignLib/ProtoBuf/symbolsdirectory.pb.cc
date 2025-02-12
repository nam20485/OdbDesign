// Generated by the protocol buffer compiler.  DO NOT EDIT!
// NO CHECKED-IN PROTOBUF GENCODE
// source: symbolsdirectory.proto
// Protobuf C++ Version: 5.29.2

#include "symbolsdirectory.pb.h"

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

inline constexpr SymbolsDirectory::Impl_::Impl_(
    ::_pbi::ConstantInitialized) noexcept
      : _cached_size_{0},
        name_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        path_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        attrlistfile_{nullptr},
        featurefile_{nullptr} {}

template <typename>
PROTOBUF_CONSTEXPR SymbolsDirectory::SymbolsDirectory(::_pbi::ConstantInitialized)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(_class_data_.base()),
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(),
#endif  // PROTOBUF_CUSTOM_VTABLE
      _impl_(::_pbi::ConstantInitialized()) {
}
struct SymbolsDirectoryDefaultTypeInternal {
  PROTOBUF_CONSTEXPR SymbolsDirectoryDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~SymbolsDirectoryDefaultTypeInternal() {}
  union {
    SymbolsDirectory _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ODBDESIGN_EXPORT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 SymbolsDirectoryDefaultTypeInternal _SymbolsDirectory_default_instance_;
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
static constexpr const ::_pb::EnumDescriptor**
    file_level_enum_descriptors_symbolsdirectory_2eproto = nullptr;
static constexpr const ::_pb::ServiceDescriptor**
    file_level_service_descriptors_symbolsdirectory_2eproto = nullptr;
const ::uint32_t
    TableStruct_symbolsdirectory_2eproto::offsets[] ABSL_ATTRIBUTE_SECTION_VARIABLE(
        protodesc_cold) = {
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::SymbolsDirectory, _impl_._has_bits_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::SymbolsDirectory, _internal_metadata_),
        ~0u,  // no _extensions_
        ~0u,  // no _oneof_case_
        ~0u,  // no _weak_field_map_
        ~0u,  // no _inlined_string_donated_
        ~0u,  // no _split_
        ~0u,  // no sizeof(Split)
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::SymbolsDirectory, _impl_.name_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::SymbolsDirectory, _impl_.path_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::SymbolsDirectory, _impl_.attrlistfile_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::SymbolsDirectory, _impl_.featurefile_),
        0,
        1,
        2,
        3,
};

static const ::_pbi::MigrationSchema
    schemas[] ABSL_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
        {0, 12, -1, sizeof(::Odb::Lib::Protobuf::SymbolsDirectory)},
};
static const ::_pb::Message* const file_default_instances[] = {
    &::Odb::Lib::Protobuf::_SymbolsDirectory_default_instance_._instance,
};
const char descriptor_table_protodef_symbolsdirectory_2eproto[] ABSL_ATTRIBUTE_SECTION_VARIABLE(
    protodesc_cold) = {
    "\n\026symbolsdirectory.proto\022\020Odb.Lib.Protob"
    "uf\032\022attrlistfile.proto\032\022featuresfile.pro"
    "to\"\340\001\n\020SymbolsDirectory\022\021\n\004name\030\001 \001(\tH\000\210"
    "\001\001\022\021\n\004path\030\002 \001(\tH\001\210\001\001\0229\n\014attrlistFile\030\003 "
    "\001(\0132\036.Odb.Lib.Protobuf.AttrListFileH\002\210\001\001"
    "\0228\n\013featureFile\030\004 \001(\0132\036.Odb.Lib.Protobuf"
    ".FeaturesFileH\003\210\001\001B\007\n\005_nameB\007\n\005_pathB\017\n\r"
    "_attrlistFileB\016\n\014_featureFileb\006proto3"
};
static const ::_pbi::DescriptorTable* const descriptor_table_symbolsdirectory_2eproto_deps[2] =
    {
        &::descriptor_table_attrlistfile_2eproto,
        &::descriptor_table_featuresfile_2eproto,
};
static ::absl::once_flag descriptor_table_symbolsdirectory_2eproto_once;
PROTOBUF_CONSTINIT const ::_pbi::DescriptorTable descriptor_table_symbolsdirectory_2eproto = {
    false,
    false,
    317,
    descriptor_table_protodef_symbolsdirectory_2eproto,
    "symbolsdirectory.proto",
    &descriptor_table_symbolsdirectory_2eproto_once,
    descriptor_table_symbolsdirectory_2eproto_deps,
    2,
    1,
    schemas,
    file_default_instances,
    TableStruct_symbolsdirectory_2eproto::offsets,
    file_level_enum_descriptors_symbolsdirectory_2eproto,
    file_level_service_descriptors_symbolsdirectory_2eproto,
};
namespace Odb {
namespace Lib {
namespace Protobuf {
// ===================================================================

class SymbolsDirectory::_Internal {
 public:
  using HasBits =
      decltype(std::declval<SymbolsDirectory>()._impl_._has_bits_);
  static constexpr ::int32_t kHasBitsOffset =
      8 * PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_._has_bits_);
};

void SymbolsDirectory::clear_attrlistfile() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (_impl_.attrlistfile_ != nullptr) _impl_.attrlistfile_->Clear();
  _impl_._has_bits_[0] &= ~0x00000004u;
}
void SymbolsDirectory::clear_featurefile() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (_impl_.featurefile_ != nullptr) _impl_.featurefile_->Clear();
  _impl_._has_bits_[0] &= ~0x00000008u;
}
SymbolsDirectory::SymbolsDirectory(::google::protobuf::Arena* arena)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  SharedCtor(arena);
  // @@protoc_insertion_point(arena_constructor:Odb.Lib.Protobuf.SymbolsDirectory)
}
inline PROTOBUF_NDEBUG_INLINE SymbolsDirectory::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility, ::google::protobuf::Arena* arena,
    const Impl_& from, const ::Odb::Lib::Protobuf::SymbolsDirectory& from_msg)
      : _has_bits_{from._has_bits_},
        _cached_size_{0},
        name_(arena, from.name_),
        path_(arena, from.path_) {}

SymbolsDirectory::SymbolsDirectory(
    ::google::protobuf::Arena* arena,
    const SymbolsDirectory& from)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  SymbolsDirectory* const _this = this;
  (void)_this;
  _internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(
      from._internal_metadata_);
  new (&_impl_) Impl_(internal_visibility(), arena, from._impl_, from);
  ::uint32_t cached_has_bits = _impl_._has_bits_[0];
  _impl_.attrlistfile_ = (cached_has_bits & 0x00000004u) ? ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::AttrListFile>(
                              arena, *from._impl_.attrlistfile_)
                        : nullptr;
  _impl_.featurefile_ = (cached_has_bits & 0x00000008u) ? ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::FeaturesFile>(
                              arena, *from._impl_.featurefile_)
                        : nullptr;

  // @@protoc_insertion_point(copy_constructor:Odb.Lib.Protobuf.SymbolsDirectory)
}
inline PROTOBUF_NDEBUG_INLINE SymbolsDirectory::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility,
    ::google::protobuf::Arena* arena)
      : _cached_size_{0},
        name_(arena),
        path_(arena) {}

inline void SymbolsDirectory::SharedCtor(::_pb::Arena* arena) {
  new (&_impl_) Impl_(internal_visibility(), arena);
  ::memset(reinterpret_cast<char *>(&_impl_) +
               offsetof(Impl_, attrlistfile_),
           0,
           offsetof(Impl_, featurefile_) -
               offsetof(Impl_, attrlistfile_) +
               sizeof(Impl_::featurefile_));
}
SymbolsDirectory::~SymbolsDirectory() {
  // @@protoc_insertion_point(destructor:Odb.Lib.Protobuf.SymbolsDirectory)
  SharedDtor(*this);
}
inline void SymbolsDirectory::SharedDtor(MessageLite& self) {
  SymbolsDirectory& this_ = static_cast<SymbolsDirectory&>(self);
  this_._internal_metadata_.Delete<::google::protobuf::UnknownFieldSet>();
  ABSL_DCHECK(this_.GetArena() == nullptr);
  this_._impl_.name_.Destroy();
  this_._impl_.path_.Destroy();
  delete this_._impl_.attrlistfile_;
  delete this_._impl_.featurefile_;
  this_._impl_.~Impl_();
}

inline void* SymbolsDirectory::PlacementNew_(const void*, void* mem,
                                        ::google::protobuf::Arena* arena) {
  return ::new (mem) SymbolsDirectory(arena);
}
constexpr auto SymbolsDirectory::InternalNewImpl_() {
  return ::google::protobuf::internal::MessageCreator::CopyInit(sizeof(SymbolsDirectory),
                                            alignof(SymbolsDirectory));
}
PROTOBUF_CONSTINIT
PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::google::protobuf::internal::ClassDataFull SymbolsDirectory::_class_data_ = {
    ::google::protobuf::internal::ClassData{
        &_SymbolsDirectory_default_instance_._instance,
        &_table_.header,
        nullptr,  // OnDemandRegisterArenaDtor
        nullptr,  // IsInitialized
        &SymbolsDirectory::MergeImpl,
        ::google::protobuf::Message::GetNewImpl<SymbolsDirectory>(),
#if defined(PROTOBUF_CUSTOM_VTABLE)
        &SymbolsDirectory::SharedDtor,
        ::google::protobuf::Message::GetClearImpl<SymbolsDirectory>(), &SymbolsDirectory::ByteSizeLong,
            &SymbolsDirectory::_InternalSerialize,
#endif  // PROTOBUF_CUSTOM_VTABLE
        PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_._cached_size_),
        false,
    },
    &SymbolsDirectory::kDescriptorMethods,
    &descriptor_table_symbolsdirectory_2eproto,
    nullptr,  // tracker
};
const ::google::protobuf::internal::ClassData* SymbolsDirectory::GetClassData() const {
  ::google::protobuf::internal::PrefetchToLocalCache(&_class_data_);
  ::google::protobuf::internal::PrefetchToLocalCache(_class_data_.tc_table);
  return _class_data_.base();
}
PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<2, 4, 2, 50, 2> SymbolsDirectory::_table_ = {
  {
    PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_._has_bits_),
    0, // no _extensions_
    4, 24,  // max_field_number, fast_idx_mask
    offsetof(decltype(_table_), field_lookup_table),
    4294967280,  // skipmap
    offsetof(decltype(_table_), field_entries),
    4,  // num_field_entries
    2,  // num_aux_entries
    offsetof(decltype(_table_), aux_entries),
    _class_data_.base(),
    nullptr,  // post_loop_handler
    ::_pbi::TcParser::GenericFallback,  // fallback
    #ifdef PROTOBUF_PREFETCH_PARSE_TABLE
    ::_pbi::TcParser::GetTable<::Odb::Lib::Protobuf::SymbolsDirectory>(),  // to_prefetch
    #endif  // PROTOBUF_PREFETCH_PARSE_TABLE
  }, {{
    // optional .Odb.Lib.Protobuf.FeaturesFile featureFile = 4;
    {::_pbi::TcParser::FastMtS1,
     {34, 3, 1, PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_.featurefile_)}},
    // optional string name = 1;
    {::_pbi::TcParser::FastUS1,
     {10, 0, 0, PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_.name_)}},
    // optional string path = 2;
    {::_pbi::TcParser::FastUS1,
     {18, 1, 0, PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_.path_)}},
    // optional .Odb.Lib.Protobuf.AttrListFile attrlistFile = 3;
    {::_pbi::TcParser::FastMtS1,
     {26, 2, 0, PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_.attrlistfile_)}},
  }}, {{
    65535, 65535
  }}, {{
    // optional string name = 1;
    {PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_.name_), _Internal::kHasBitsOffset + 0, 0,
    (0 | ::_fl::kFcOptional | ::_fl::kUtf8String | ::_fl::kRepAString)},
    // optional string path = 2;
    {PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_.path_), _Internal::kHasBitsOffset + 1, 0,
    (0 | ::_fl::kFcOptional | ::_fl::kUtf8String | ::_fl::kRepAString)},
    // optional .Odb.Lib.Protobuf.AttrListFile attrlistFile = 3;
    {PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_.attrlistfile_), _Internal::kHasBitsOffset + 2, 0,
    (0 | ::_fl::kFcOptional | ::_fl::kMessage | ::_fl::kTvTable)},
    // optional .Odb.Lib.Protobuf.FeaturesFile featureFile = 4;
    {PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_.featurefile_), _Internal::kHasBitsOffset + 3, 1,
    (0 | ::_fl::kFcOptional | ::_fl::kMessage | ::_fl::kTvTable)},
  }}, {{
    {::_pbi::TcParser::GetTable<::Odb::Lib::Protobuf::AttrListFile>()},
    {::_pbi::TcParser::GetTable<::Odb::Lib::Protobuf::FeaturesFile>()},
  }}, {{
    "\41\4\4\0\0\0\0\0"
    "Odb.Lib.Protobuf.SymbolsDirectory"
    "name"
    "path"
  }},
};

PROTOBUF_NOINLINE void SymbolsDirectory::Clear() {
// @@protoc_insertion_point(message_clear_start:Odb.Lib.Protobuf.SymbolsDirectory)
  ::google::protobuf::internal::TSanWrite(&_impl_);
  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _impl_._has_bits_[0];
  if (cached_has_bits & 0x0000000fu) {
    if (cached_has_bits & 0x00000001u) {
      _impl_.name_.ClearNonDefaultToEmpty();
    }
    if (cached_has_bits & 0x00000002u) {
      _impl_.path_.ClearNonDefaultToEmpty();
    }
    if (cached_has_bits & 0x00000004u) {
      ABSL_DCHECK(_impl_.attrlistfile_ != nullptr);
      _impl_.attrlistfile_->Clear();
    }
    if (cached_has_bits & 0x00000008u) {
      ABSL_DCHECK(_impl_.featurefile_ != nullptr);
      _impl_.featurefile_->Clear();
    }
  }
  _impl_._has_bits_.Clear();
  _internal_metadata_.Clear<::google::protobuf::UnknownFieldSet>();
}

#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::uint8_t* SymbolsDirectory::_InternalSerialize(
            const MessageLite& base, ::uint8_t* target,
            ::google::protobuf::io::EpsCopyOutputStream* stream) {
          const SymbolsDirectory& this_ = static_cast<const SymbolsDirectory&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::uint8_t* SymbolsDirectory::_InternalSerialize(
            ::uint8_t* target,
            ::google::protobuf::io::EpsCopyOutputStream* stream) const {
          const SymbolsDirectory& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          // @@protoc_insertion_point(serialize_to_array_start:Odb.Lib.Protobuf.SymbolsDirectory)
          ::uint32_t cached_has_bits = 0;
          (void)cached_has_bits;

          cached_has_bits = this_._impl_._has_bits_[0];
          // optional string name = 1;
          if (cached_has_bits & 0x00000001u) {
            const std::string& _s = this_._internal_name();
            ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "Odb.Lib.Protobuf.SymbolsDirectory.name");
            target = stream->WriteStringMaybeAliased(1, _s, target);
          }

          // optional string path = 2;
          if (cached_has_bits & 0x00000002u) {
            const std::string& _s = this_._internal_path();
            ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "Odb.Lib.Protobuf.SymbolsDirectory.path");
            target = stream->WriteStringMaybeAliased(2, _s, target);
          }

          // optional .Odb.Lib.Protobuf.AttrListFile attrlistFile = 3;
          if (cached_has_bits & 0x00000004u) {
            target = ::google::protobuf::internal::WireFormatLite::InternalWriteMessage(
                3, *this_._impl_.attrlistfile_, this_._impl_.attrlistfile_->GetCachedSize(), target,
                stream);
          }

          // optional .Odb.Lib.Protobuf.FeaturesFile featureFile = 4;
          if (cached_has_bits & 0x00000008u) {
            target = ::google::protobuf::internal::WireFormatLite::InternalWriteMessage(
                4, *this_._impl_.featurefile_, this_._impl_.featurefile_->GetCachedSize(), target,
                stream);
          }

          if (PROTOBUF_PREDICT_FALSE(this_._internal_metadata_.have_unknown_fields())) {
            target =
                ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
                    this_._internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance), target, stream);
          }
          // @@protoc_insertion_point(serialize_to_array_end:Odb.Lib.Protobuf.SymbolsDirectory)
          return target;
        }

#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::size_t SymbolsDirectory::ByteSizeLong(const MessageLite& base) {
          const SymbolsDirectory& this_ = static_cast<const SymbolsDirectory&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::size_t SymbolsDirectory::ByteSizeLong() const {
          const SymbolsDirectory& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          // @@protoc_insertion_point(message_byte_size_start:Odb.Lib.Protobuf.SymbolsDirectory)
          ::size_t total_size = 0;

          ::uint32_t cached_has_bits = 0;
          // Prevent compiler warnings about cached_has_bits being unused
          (void)cached_has_bits;

          ::_pbi::Prefetch5LinesFrom7Lines(&this_);
          cached_has_bits = this_._impl_._has_bits_[0];
          if (cached_has_bits & 0x0000000fu) {
            // optional string name = 1;
            if (cached_has_bits & 0x00000001u) {
              total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                              this_._internal_name());
            }
            // optional string path = 2;
            if (cached_has_bits & 0x00000002u) {
              total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                              this_._internal_path());
            }
            // optional .Odb.Lib.Protobuf.AttrListFile attrlistFile = 3;
            if (cached_has_bits & 0x00000004u) {
              total_size += 1 +
                            ::google::protobuf::internal::WireFormatLite::MessageSize(*this_._impl_.attrlistfile_);
            }
            // optional .Odb.Lib.Protobuf.FeaturesFile featureFile = 4;
            if (cached_has_bits & 0x00000008u) {
              total_size += 1 +
                            ::google::protobuf::internal::WireFormatLite::MessageSize(*this_._impl_.featurefile_);
            }
          }
          return this_.MaybeComputeUnknownFieldsSize(total_size,
                                                     &this_._impl_._cached_size_);
        }

void SymbolsDirectory::MergeImpl(::google::protobuf::MessageLite& to_msg, const ::google::protobuf::MessageLite& from_msg) {
  auto* const _this = static_cast<SymbolsDirectory*>(&to_msg);
  auto& from = static_cast<const SymbolsDirectory&>(from_msg);
  ::google::protobuf::Arena* arena = _this->GetArena();
  // @@protoc_insertion_point(class_specific_merge_from_start:Odb.Lib.Protobuf.SymbolsDirectory)
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = from._impl_._has_bits_[0];
  if (cached_has_bits & 0x0000000fu) {
    if (cached_has_bits & 0x00000001u) {
      _this->_internal_set_name(from._internal_name());
    }
    if (cached_has_bits & 0x00000002u) {
      _this->_internal_set_path(from._internal_path());
    }
    if (cached_has_bits & 0x00000004u) {
      ABSL_DCHECK(from._impl_.attrlistfile_ != nullptr);
      if (_this->_impl_.attrlistfile_ == nullptr) {
        _this->_impl_.attrlistfile_ =
            ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::AttrListFile>(arena, *from._impl_.attrlistfile_);
      } else {
        _this->_impl_.attrlistfile_->MergeFrom(*from._impl_.attrlistfile_);
      }
    }
    if (cached_has_bits & 0x00000008u) {
      ABSL_DCHECK(from._impl_.featurefile_ != nullptr);
      if (_this->_impl_.featurefile_ == nullptr) {
        _this->_impl_.featurefile_ =
            ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::FeaturesFile>(arena, *from._impl_.featurefile_);
      } else {
        _this->_impl_.featurefile_->MergeFrom(*from._impl_.featurefile_);
      }
    }
  }
  _this->_impl_._has_bits_[0] |= cached_has_bits;
  _this->_internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(from._internal_metadata_);
}

void SymbolsDirectory::CopyFrom(const SymbolsDirectory& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:Odb.Lib.Protobuf.SymbolsDirectory)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}


void SymbolsDirectory::InternalSwap(SymbolsDirectory* PROTOBUF_RESTRICT other) {
  using std::swap;
  auto* arena = GetArena();
  ABSL_DCHECK_EQ(arena, other->GetArena());
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  swap(_impl_._has_bits_[0], other->_impl_._has_bits_[0]);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.name_, &other->_impl_.name_, arena);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.path_, &other->_impl_.path_, arena);
  ::google::protobuf::internal::memswap<
      PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_.featurefile_)
      + sizeof(SymbolsDirectory::_impl_.featurefile_)
      - PROTOBUF_FIELD_OFFSET(SymbolsDirectory, _impl_.attrlistfile_)>(
          reinterpret_cast<char*>(&_impl_.attrlistfile_),
          reinterpret_cast<char*>(&other->_impl_.attrlistfile_));
}

::google::protobuf::Metadata SymbolsDirectory::GetMetadata() const {
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
        (::_pbi::AddDescriptors(&descriptor_table_symbolsdirectory_2eproto),
         ::std::false_type{});
#include "google/protobuf/port_undef.inc"
