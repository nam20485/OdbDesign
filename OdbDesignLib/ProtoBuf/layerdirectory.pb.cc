// Generated by the protocol buffer compiler.  DO NOT EDIT!
// NO CHECKED-IN PROTOBUF GENCODE
// source: layerdirectory.proto
// Protobuf C++ Version: 5.29.2

#include "layerdirectory.pb.h"

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

inline constexpr LayerDirectory::Impl_::Impl_(
    ::_pbi::ConstantInitialized) noexcept
      : _cached_size_{0},
        name_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        path_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        components_{nullptr},
        attrlistfile_{nullptr},
        featurefile_{nullptr} {}

template <typename>
PROTOBUF_CONSTEXPR LayerDirectory::LayerDirectory(::_pbi::ConstantInitialized)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(_class_data_.base()),
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(),
#endif  // PROTOBUF_CUSTOM_VTABLE
      _impl_(::_pbi::ConstantInitialized()) {
}
struct LayerDirectoryDefaultTypeInternal {
  PROTOBUF_CONSTEXPR LayerDirectoryDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~LayerDirectoryDefaultTypeInternal() {}
  union {
    LayerDirectory _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ODBDESIGN_EXPORT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 LayerDirectoryDefaultTypeInternal _LayerDirectory_default_instance_;
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
static constexpr const ::_pb::EnumDescriptor**
    file_level_enum_descriptors_layerdirectory_2eproto = nullptr;
static constexpr const ::_pb::ServiceDescriptor**
    file_level_service_descriptors_layerdirectory_2eproto = nullptr;
const ::uint32_t
    TableStruct_layerdirectory_2eproto::offsets[] ABSL_ATTRIBUTE_SECTION_VARIABLE(
        protodesc_cold) = {
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::LayerDirectory, _impl_._has_bits_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::LayerDirectory, _internal_metadata_),
        ~0u,  // no _extensions_
        ~0u,  // no _oneof_case_
        ~0u,  // no _weak_field_map_
        ~0u,  // no _inlined_string_donated_
        ~0u,  // no _split_
        ~0u,  // no sizeof(Split)
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::LayerDirectory, _impl_.name_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::LayerDirectory, _impl_.path_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::LayerDirectory, _impl_.components_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::LayerDirectory, _impl_.attrlistfile_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::LayerDirectory, _impl_.featurefile_),
        0,
        1,
        2,
        3,
        4,
};

static const ::_pbi::MigrationSchema
    schemas[] ABSL_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
        {0, 13, -1, sizeof(::Odb::Lib::Protobuf::LayerDirectory)},
};
static const ::_pb::Message* const file_default_instances[] = {
    &::Odb::Lib::Protobuf::_LayerDirectory_default_instance_._instance,
};
const char descriptor_table_protodef_layerdirectory_2eproto[] ABSL_ATTRIBUTE_SECTION_VARIABLE(
    protodesc_cold) = {
    "\n\024layerdirectory.proto\022\020Odb.Lib.Protobuf"
    "\032\024componentsfile.proto\032\022attrlistfile.pro"
    "to\032\022featuresfile.proto\"\250\002\n\016LayerDirector"
    "y\022\021\n\004name\030\001 \001(\tH\000\210\001\001\022\021\n\004path\030\002 \001(\tH\001\210\001\001\022"
    "9\n\ncomponents\030\003 \001(\0132 .Odb.Lib.Protobuf.C"
    "omponentsFileH\002\210\001\001\0229\n\014attrlistFile\030\004 \001(\013"
    "2\036.Odb.Lib.Protobuf.AttrListFileH\003\210\001\001\0228\n"
    "\013featureFile\030\005 \001(\0132\036.Odb.Lib.Protobuf.Fe"
    "aturesFileH\004\210\001\001B\007\n\005_nameB\007\n\005_pathB\r\n\013_co"
    "mponentsB\017\n\r_attrlistFileB\016\n\014_featureFil"
    "eb\006proto3"
};
static const ::_pbi::DescriptorTable* const descriptor_table_layerdirectory_2eproto_deps[3] =
    {
        &::descriptor_table_attrlistfile_2eproto,
        &::descriptor_table_componentsfile_2eproto,
        &::descriptor_table_featuresfile_2eproto,
};
static ::absl::once_flag descriptor_table_layerdirectory_2eproto_once;
PROTOBUF_CONSTINIT const ::_pbi::DescriptorTable descriptor_table_layerdirectory_2eproto = {
    false,
    false,
    409,
    descriptor_table_protodef_layerdirectory_2eproto,
    "layerdirectory.proto",
    &descriptor_table_layerdirectory_2eproto_once,
    descriptor_table_layerdirectory_2eproto_deps,
    3,
    1,
    schemas,
    file_default_instances,
    TableStruct_layerdirectory_2eproto::offsets,
    file_level_enum_descriptors_layerdirectory_2eproto,
    file_level_service_descriptors_layerdirectory_2eproto,
};
namespace Odb {
namespace Lib {
namespace Protobuf {
// ===================================================================

class LayerDirectory::_Internal {
 public:
  using HasBits =
      decltype(std::declval<LayerDirectory>()._impl_._has_bits_);
  static constexpr ::int32_t kHasBitsOffset =
      8 * PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_._has_bits_);
};

void LayerDirectory::clear_components() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (_impl_.components_ != nullptr) _impl_.components_->Clear();
  _impl_._has_bits_[0] &= ~0x00000004u;
}
void LayerDirectory::clear_attrlistfile() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (_impl_.attrlistfile_ != nullptr) _impl_.attrlistfile_->Clear();
  _impl_._has_bits_[0] &= ~0x00000008u;
}
void LayerDirectory::clear_featurefile() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (_impl_.featurefile_ != nullptr) _impl_.featurefile_->Clear();
  _impl_._has_bits_[0] &= ~0x00000010u;
}
LayerDirectory::LayerDirectory(::google::protobuf::Arena* arena)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  SharedCtor(arena);
  // @@protoc_insertion_point(arena_constructor:Odb.Lib.Protobuf.LayerDirectory)
}
inline PROTOBUF_NDEBUG_INLINE LayerDirectory::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility, ::google::protobuf::Arena* arena,
    const Impl_& from, const ::Odb::Lib::Protobuf::LayerDirectory& from_msg)
      : _has_bits_{from._has_bits_},
        _cached_size_{0},
        name_(arena, from.name_),
        path_(arena, from.path_) {}

LayerDirectory::LayerDirectory(
    ::google::protobuf::Arena* arena,
    const LayerDirectory& from)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  LayerDirectory* const _this = this;
  (void)_this;
  _internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(
      from._internal_metadata_);
  new (&_impl_) Impl_(internal_visibility(), arena, from._impl_, from);
  ::uint32_t cached_has_bits = _impl_._has_bits_[0];
  _impl_.components_ = (cached_has_bits & 0x00000004u) ? ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::ComponentsFile>(
                              arena, *from._impl_.components_)
                        : nullptr;
  _impl_.attrlistfile_ = (cached_has_bits & 0x00000008u) ? ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::AttrListFile>(
                              arena, *from._impl_.attrlistfile_)
                        : nullptr;
  _impl_.featurefile_ = (cached_has_bits & 0x00000010u) ? ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::FeaturesFile>(
                              arena, *from._impl_.featurefile_)
                        : nullptr;

  // @@protoc_insertion_point(copy_constructor:Odb.Lib.Protobuf.LayerDirectory)
}
inline PROTOBUF_NDEBUG_INLINE LayerDirectory::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility,
    ::google::protobuf::Arena* arena)
      : _cached_size_{0},
        name_(arena),
        path_(arena) {}

inline void LayerDirectory::SharedCtor(::_pb::Arena* arena) {
  new (&_impl_) Impl_(internal_visibility(), arena);
  ::memset(reinterpret_cast<char *>(&_impl_) +
               offsetof(Impl_, components_),
           0,
           offsetof(Impl_, featurefile_) -
               offsetof(Impl_, components_) +
               sizeof(Impl_::featurefile_));
}
LayerDirectory::~LayerDirectory() {
  // @@protoc_insertion_point(destructor:Odb.Lib.Protobuf.LayerDirectory)
  SharedDtor(*this);
}
inline void LayerDirectory::SharedDtor(MessageLite& self) {
  LayerDirectory& this_ = static_cast<LayerDirectory&>(self);
  this_._internal_metadata_.Delete<::google::protobuf::UnknownFieldSet>();
  ABSL_DCHECK(this_.GetArena() == nullptr);
  this_._impl_.name_.Destroy();
  this_._impl_.path_.Destroy();
  delete this_._impl_.components_;
  delete this_._impl_.attrlistfile_;
  delete this_._impl_.featurefile_;
  this_._impl_.~Impl_();
}

inline void* LayerDirectory::PlacementNew_(const void*, void* mem,
                                        ::google::protobuf::Arena* arena) {
  return ::new (mem) LayerDirectory(arena);
}
constexpr auto LayerDirectory::InternalNewImpl_() {
  return ::google::protobuf::internal::MessageCreator::CopyInit(sizeof(LayerDirectory),
                                            alignof(LayerDirectory));
}
PROTOBUF_CONSTINIT
PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::google::protobuf::internal::ClassDataFull LayerDirectory::_class_data_ = {
    ::google::protobuf::internal::ClassData{
        &_LayerDirectory_default_instance_._instance,
        &_table_.header,
        nullptr,  // OnDemandRegisterArenaDtor
        nullptr,  // IsInitialized
        &LayerDirectory::MergeImpl,
        ::google::protobuf::Message::GetNewImpl<LayerDirectory>(),
#if defined(PROTOBUF_CUSTOM_VTABLE)
        &LayerDirectory::SharedDtor,
        ::google::protobuf::Message::GetClearImpl<LayerDirectory>(), &LayerDirectory::ByteSizeLong,
            &LayerDirectory::_InternalSerialize,
#endif  // PROTOBUF_CUSTOM_VTABLE
        PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_._cached_size_),
        false,
    },
    &LayerDirectory::kDescriptorMethods,
    &descriptor_table_layerdirectory_2eproto,
    nullptr,  // tracker
};
const ::google::protobuf::internal::ClassData* LayerDirectory::GetClassData() const {
  ::google::protobuf::internal::PrefetchToLocalCache(&_class_data_);
  ::google::protobuf::internal::PrefetchToLocalCache(_class_data_.tc_table);
  return _class_data_.base();
}
PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<3, 5, 3, 48, 2> LayerDirectory::_table_ = {
  {
    PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_._has_bits_),
    0, // no _extensions_
    5, 56,  // max_field_number, fast_idx_mask
    offsetof(decltype(_table_), field_lookup_table),
    4294967264,  // skipmap
    offsetof(decltype(_table_), field_entries),
    5,  // num_field_entries
    3,  // num_aux_entries
    offsetof(decltype(_table_), aux_entries),
    _class_data_.base(),
    nullptr,  // post_loop_handler
    ::_pbi::TcParser::GenericFallback,  // fallback
    #ifdef PROTOBUF_PREFETCH_PARSE_TABLE
    ::_pbi::TcParser::GetTable<::Odb::Lib::Protobuf::LayerDirectory>(),  // to_prefetch
    #endif  // PROTOBUF_PREFETCH_PARSE_TABLE
  }, {{
    {::_pbi::TcParser::MiniParse, {}},
    // optional string name = 1;
    {::_pbi::TcParser::FastUS1,
     {10, 0, 0, PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_.name_)}},
    // optional string path = 2;
    {::_pbi::TcParser::FastUS1,
     {18, 1, 0, PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_.path_)}},
    // optional .Odb.Lib.Protobuf.ComponentsFile components = 3;
    {::_pbi::TcParser::FastMtS1,
     {26, 2, 0, PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_.components_)}},
    // optional .Odb.Lib.Protobuf.AttrListFile attrlistFile = 4;
    {::_pbi::TcParser::FastMtS1,
     {34, 3, 1, PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_.attrlistfile_)}},
    // optional .Odb.Lib.Protobuf.FeaturesFile featureFile = 5;
    {::_pbi::TcParser::FastMtS1,
     {42, 4, 2, PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_.featurefile_)}},
    {::_pbi::TcParser::MiniParse, {}},
    {::_pbi::TcParser::MiniParse, {}},
  }}, {{
    65535, 65535
  }}, {{
    // optional string name = 1;
    {PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_.name_), _Internal::kHasBitsOffset + 0, 0,
    (0 | ::_fl::kFcOptional | ::_fl::kUtf8String | ::_fl::kRepAString)},
    // optional string path = 2;
    {PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_.path_), _Internal::kHasBitsOffset + 1, 0,
    (0 | ::_fl::kFcOptional | ::_fl::kUtf8String | ::_fl::kRepAString)},
    // optional .Odb.Lib.Protobuf.ComponentsFile components = 3;
    {PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_.components_), _Internal::kHasBitsOffset + 2, 0,
    (0 | ::_fl::kFcOptional | ::_fl::kMessage | ::_fl::kTvTable)},
    // optional .Odb.Lib.Protobuf.AttrListFile attrlistFile = 4;
    {PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_.attrlistfile_), _Internal::kHasBitsOffset + 3, 1,
    (0 | ::_fl::kFcOptional | ::_fl::kMessage | ::_fl::kTvTable)},
    // optional .Odb.Lib.Protobuf.FeaturesFile featureFile = 5;
    {PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_.featurefile_), _Internal::kHasBitsOffset + 4, 2,
    (0 | ::_fl::kFcOptional | ::_fl::kMessage | ::_fl::kTvTable)},
  }}, {{
    {::_pbi::TcParser::GetTable<::Odb::Lib::Protobuf::ComponentsFile>()},
    {::_pbi::TcParser::GetTable<::Odb::Lib::Protobuf::AttrListFile>()},
    {::_pbi::TcParser::GetTable<::Odb::Lib::Protobuf::FeaturesFile>()},
  }}, {{
    "\37\4\4\0\0\0\0\0"
    "Odb.Lib.Protobuf.LayerDirectory"
    "name"
    "path"
  }},
};

PROTOBUF_NOINLINE void LayerDirectory::Clear() {
// @@protoc_insertion_point(message_clear_start:Odb.Lib.Protobuf.LayerDirectory)
  ::google::protobuf::internal::TSanWrite(&_impl_);
  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _impl_._has_bits_[0];
  if (cached_has_bits & 0x0000001fu) {
    if (cached_has_bits & 0x00000001u) {
      _impl_.name_.ClearNonDefaultToEmpty();
    }
    if (cached_has_bits & 0x00000002u) {
      _impl_.path_.ClearNonDefaultToEmpty();
    }
    if (cached_has_bits & 0x00000004u) {
      ABSL_DCHECK(_impl_.components_ != nullptr);
      _impl_.components_->Clear();
    }
    if (cached_has_bits & 0x00000008u) {
      ABSL_DCHECK(_impl_.attrlistfile_ != nullptr);
      _impl_.attrlistfile_->Clear();
    }
    if (cached_has_bits & 0x00000010u) {
      ABSL_DCHECK(_impl_.featurefile_ != nullptr);
      _impl_.featurefile_->Clear();
    }
  }
  _impl_._has_bits_.Clear();
  _internal_metadata_.Clear<::google::protobuf::UnknownFieldSet>();
}

#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::uint8_t* LayerDirectory::_InternalSerialize(
            const MessageLite& base, ::uint8_t* target,
            ::google::protobuf::io::EpsCopyOutputStream* stream) {
          const LayerDirectory& this_ = static_cast<const LayerDirectory&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::uint8_t* LayerDirectory::_InternalSerialize(
            ::uint8_t* target,
            ::google::protobuf::io::EpsCopyOutputStream* stream) const {
          const LayerDirectory& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          // @@protoc_insertion_point(serialize_to_array_start:Odb.Lib.Protobuf.LayerDirectory)
          ::uint32_t cached_has_bits = 0;
          (void)cached_has_bits;

          cached_has_bits = this_._impl_._has_bits_[0];
          // optional string name = 1;
          if (cached_has_bits & 0x00000001u) {
            const std::string& _s = this_._internal_name();
            ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "Odb.Lib.Protobuf.LayerDirectory.name");
            target = stream->WriteStringMaybeAliased(1, _s, target);
          }

          // optional string path = 2;
          if (cached_has_bits & 0x00000002u) {
            const std::string& _s = this_._internal_path();
            ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "Odb.Lib.Protobuf.LayerDirectory.path");
            target = stream->WriteStringMaybeAliased(2, _s, target);
          }

          // optional .Odb.Lib.Protobuf.ComponentsFile components = 3;
          if (cached_has_bits & 0x00000004u) {
            target = ::google::protobuf::internal::WireFormatLite::InternalWriteMessage(
                3, *this_._impl_.components_, this_._impl_.components_->GetCachedSize(), target,
                stream);
          }

          // optional .Odb.Lib.Protobuf.AttrListFile attrlistFile = 4;
          if (cached_has_bits & 0x00000008u) {
            target = ::google::protobuf::internal::WireFormatLite::InternalWriteMessage(
                4, *this_._impl_.attrlistfile_, this_._impl_.attrlistfile_->GetCachedSize(), target,
                stream);
          }

          // optional .Odb.Lib.Protobuf.FeaturesFile featureFile = 5;
          if (cached_has_bits & 0x00000010u) {
            target = ::google::protobuf::internal::WireFormatLite::InternalWriteMessage(
                5, *this_._impl_.featurefile_, this_._impl_.featurefile_->GetCachedSize(), target,
                stream);
          }

          if (PROTOBUF_PREDICT_FALSE(this_._internal_metadata_.have_unknown_fields())) {
            target =
                ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
                    this_._internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance), target, stream);
          }
          // @@protoc_insertion_point(serialize_to_array_end:Odb.Lib.Protobuf.LayerDirectory)
          return target;
        }

#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::size_t LayerDirectory::ByteSizeLong(const MessageLite& base) {
          const LayerDirectory& this_ = static_cast<const LayerDirectory&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::size_t LayerDirectory::ByteSizeLong() const {
          const LayerDirectory& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          // @@protoc_insertion_point(message_byte_size_start:Odb.Lib.Protobuf.LayerDirectory)
          ::size_t total_size = 0;

          ::uint32_t cached_has_bits = 0;
          // Prevent compiler warnings about cached_has_bits being unused
          (void)cached_has_bits;

          ::_pbi::Prefetch5LinesFrom7Lines(&this_);
          cached_has_bits = this_._impl_._has_bits_[0];
          if (cached_has_bits & 0x0000001fu) {
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
            // optional .Odb.Lib.Protobuf.ComponentsFile components = 3;
            if (cached_has_bits & 0x00000004u) {
              total_size += 1 +
                            ::google::protobuf::internal::WireFormatLite::MessageSize(*this_._impl_.components_);
            }
            // optional .Odb.Lib.Protobuf.AttrListFile attrlistFile = 4;
            if (cached_has_bits & 0x00000008u) {
              total_size += 1 +
                            ::google::protobuf::internal::WireFormatLite::MessageSize(*this_._impl_.attrlistfile_);
            }
            // optional .Odb.Lib.Protobuf.FeaturesFile featureFile = 5;
            if (cached_has_bits & 0x00000010u) {
              total_size += 1 +
                            ::google::protobuf::internal::WireFormatLite::MessageSize(*this_._impl_.featurefile_);
            }
          }
          return this_.MaybeComputeUnknownFieldsSize(total_size,
                                                     &this_._impl_._cached_size_);
        }

void LayerDirectory::MergeImpl(::google::protobuf::MessageLite& to_msg, const ::google::protobuf::MessageLite& from_msg) {
  auto* const _this = static_cast<LayerDirectory*>(&to_msg);
  auto& from = static_cast<const LayerDirectory&>(from_msg);
  ::google::protobuf::Arena* arena = _this->GetArena();
  // @@protoc_insertion_point(class_specific_merge_from_start:Odb.Lib.Protobuf.LayerDirectory)
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = from._impl_._has_bits_[0];
  if (cached_has_bits & 0x0000001fu) {
    if (cached_has_bits & 0x00000001u) {
      _this->_internal_set_name(from._internal_name());
    }
    if (cached_has_bits & 0x00000002u) {
      _this->_internal_set_path(from._internal_path());
    }
    if (cached_has_bits & 0x00000004u) {
      ABSL_DCHECK(from._impl_.components_ != nullptr);
      if (_this->_impl_.components_ == nullptr) {
        _this->_impl_.components_ =
            ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::ComponentsFile>(arena, *from._impl_.components_);
      } else {
        _this->_impl_.components_->MergeFrom(*from._impl_.components_);
      }
    }
    if (cached_has_bits & 0x00000008u) {
      ABSL_DCHECK(from._impl_.attrlistfile_ != nullptr);
      if (_this->_impl_.attrlistfile_ == nullptr) {
        _this->_impl_.attrlistfile_ =
            ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::AttrListFile>(arena, *from._impl_.attrlistfile_);
      } else {
        _this->_impl_.attrlistfile_->MergeFrom(*from._impl_.attrlistfile_);
      }
    }
    if (cached_has_bits & 0x00000010u) {
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

void LayerDirectory::CopyFrom(const LayerDirectory& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:Odb.Lib.Protobuf.LayerDirectory)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}


void LayerDirectory::InternalSwap(LayerDirectory* PROTOBUF_RESTRICT other) {
  using std::swap;
  auto* arena = GetArena();
  ABSL_DCHECK_EQ(arena, other->GetArena());
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  swap(_impl_._has_bits_[0], other->_impl_._has_bits_[0]);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.name_, &other->_impl_.name_, arena);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.path_, &other->_impl_.path_, arena);
  ::google::protobuf::internal::memswap<
      PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_.featurefile_)
      + sizeof(LayerDirectory::_impl_.featurefile_)
      - PROTOBUF_FIELD_OFFSET(LayerDirectory, _impl_.components_)>(
          reinterpret_cast<char*>(&_impl_.components_),
          reinterpret_cast<char*>(&other->_impl_.components_));
}

::google::protobuf::Metadata LayerDirectory::GetMetadata() const {
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
        (::_pbi::AddDescriptors(&descriptor_table_layerdirectory_2eproto),
         ::std::false_type{});
#include "google/protobuf/port_undef.inc"
