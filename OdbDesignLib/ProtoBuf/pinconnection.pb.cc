// Generated by the protocol buffer compiler.  DO NOT EDIT!
// NO CHECKED-IN PROTOBUF GENCODE
// source: pinconnection.proto
// Protobuf C++ Version: 5.29.2

#include "pinconnection.pb.h"

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
namespace ProductModel {

inline constexpr PinConnection::Impl_::Impl_(
    ::_pbi::ConstantInitialized) noexcept
      : _cached_size_{0},
        name_(
            &::google::protobuf::internal::fixed_address_empty_string,
            ::_pbi::ConstantInitialized()),
        component_{nullptr},
        pin_{nullptr} {}

template <typename>
PROTOBUF_CONSTEXPR PinConnection::PinConnection(::_pbi::ConstantInitialized)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(_class_data_.base()),
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(),
#endif  // PROTOBUF_CUSTOM_VTABLE
      _impl_(::_pbi::ConstantInitialized()) {
}
struct PinConnectionDefaultTypeInternal {
  PROTOBUF_CONSTEXPR PinConnectionDefaultTypeInternal() : _instance(::_pbi::ConstantInitialized{}) {}
  ~PinConnectionDefaultTypeInternal() {}
  union {
    PinConnection _instance;
  };
};

PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT ODBDESIGN_EXPORT
    PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 PinConnectionDefaultTypeInternal _PinConnection_default_instance_;
}  // namespace ProductModel
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
static constexpr const ::_pb::EnumDescriptor**
    file_level_enum_descriptors_pinconnection_2eproto = nullptr;
static constexpr const ::_pb::ServiceDescriptor**
    file_level_service_descriptors_pinconnection_2eproto = nullptr;
const ::uint32_t
    TableStruct_pinconnection_2eproto::offsets[] ABSL_ATTRIBUTE_SECTION_VARIABLE(
        protodesc_cold) = {
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::ProductModel::PinConnection, _impl_._has_bits_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::ProductModel::PinConnection, _internal_metadata_),
        ~0u,  // no _extensions_
        ~0u,  // no _oneof_case_
        ~0u,  // no _weak_field_map_
        ~0u,  // no _inlined_string_donated_
        ~0u,  // no _split_
        ~0u,  // no sizeof(Split)
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::ProductModel::PinConnection, _impl_.name_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::ProductModel::PinConnection, _impl_.component_),
        PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::ProductModel::PinConnection, _impl_.pin_),
        0,
        1,
        2,
};

static const ::_pbi::MigrationSchema
    schemas[] ABSL_ATTRIBUTE_SECTION_VARIABLE(protodesc_cold) = {
        {0, 11, -1, sizeof(::Odb::Lib::Protobuf::ProductModel::PinConnection)},
};
static const ::_pb::Message* const file_default_instances[] = {
    &::Odb::Lib::Protobuf::ProductModel::_PinConnection_default_instance_._instance,
};
const char descriptor_table_protodef_pinconnection_2eproto[] ABSL_ATTRIBUTE_SECTION_VARIABLE(
    protodesc_cold) = {
    "\n\023pinconnection.proto\022\035Odb.Lib.Protobuf."
    "ProductModel\032\017component.proto\032\tpin.proto"
    "\"\271\001\n\rPinConnection\022\021\n\004name\030\001 \001(\tH\000\210\001\001\022@\n"
    "\tcomponent\030\002 \001(\0132(.Odb.Lib.Protobuf.Prod"
    "uctModel.ComponentH\001\210\001\001\0224\n\003pin\030\003 \001(\0132\".O"
    "db.Lib.Protobuf.ProductModel.PinH\002\210\001\001B\007\n"
    "\005_nameB\014\n\n_componentB\006\n\004_pinb\006proto3"
};
static const ::_pbi::DescriptorTable* const descriptor_table_pinconnection_2eproto_deps[2] =
    {
        &::descriptor_table_component_2eproto,
        &::descriptor_table_pin_2eproto,
};
static ::absl::once_flag descriptor_table_pinconnection_2eproto_once;
PROTOBUF_CONSTINIT const ::_pbi::DescriptorTable descriptor_table_pinconnection_2eproto = {
    false,
    false,
    276,
    descriptor_table_protodef_pinconnection_2eproto,
    "pinconnection.proto",
    &descriptor_table_pinconnection_2eproto_once,
    descriptor_table_pinconnection_2eproto_deps,
    2,
    1,
    schemas,
    file_default_instances,
    TableStruct_pinconnection_2eproto::offsets,
    file_level_enum_descriptors_pinconnection_2eproto,
    file_level_service_descriptors_pinconnection_2eproto,
};
namespace Odb {
namespace Lib {
namespace Protobuf {
namespace ProductModel {
// ===================================================================

class PinConnection::_Internal {
 public:
  using HasBits =
      decltype(std::declval<PinConnection>()._impl_._has_bits_);
  static constexpr ::int32_t kHasBitsOffset =
      8 * PROTOBUF_FIELD_OFFSET(PinConnection, _impl_._has_bits_);
};

void PinConnection::clear_component() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (_impl_.component_ != nullptr) _impl_.component_->Clear();
  _impl_._has_bits_[0] &= ~0x00000002u;
}
void PinConnection::clear_pin() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (_impl_.pin_ != nullptr) _impl_.pin_->Clear();
  _impl_._has_bits_[0] &= ~0x00000004u;
}
PinConnection::PinConnection(::google::protobuf::Arena* arena)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  SharedCtor(arena);
  // @@protoc_insertion_point(arena_constructor:Odb.Lib.Protobuf.ProductModel.PinConnection)
}
inline PROTOBUF_NDEBUG_INLINE PinConnection::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility, ::google::protobuf::Arena* arena,
    const Impl_& from, const ::Odb::Lib::Protobuf::ProductModel::PinConnection& from_msg)
      : _has_bits_{from._has_bits_},
        _cached_size_{0},
        name_(arena, from.name_) {}

PinConnection::PinConnection(
    ::google::protobuf::Arena* arena,
    const PinConnection& from)
#if defined(PROTOBUF_CUSTOM_VTABLE)
    : ::google::protobuf::Message(arena, _class_data_.base()) {
#else   // PROTOBUF_CUSTOM_VTABLE
    : ::google::protobuf::Message(arena) {
#endif  // PROTOBUF_CUSTOM_VTABLE
  PinConnection* const _this = this;
  (void)_this;
  _internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(
      from._internal_metadata_);
  new (&_impl_) Impl_(internal_visibility(), arena, from._impl_, from);
  ::uint32_t cached_has_bits = _impl_._has_bits_[0];
  _impl_.component_ = (cached_has_bits & 0x00000002u) ? ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::ProductModel::Component>(
                              arena, *from._impl_.component_)
                        : nullptr;
  _impl_.pin_ = (cached_has_bits & 0x00000004u) ? ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::ProductModel::Pin>(
                              arena, *from._impl_.pin_)
                        : nullptr;

  // @@protoc_insertion_point(copy_constructor:Odb.Lib.Protobuf.ProductModel.PinConnection)
}
inline PROTOBUF_NDEBUG_INLINE PinConnection::Impl_::Impl_(
    ::google::protobuf::internal::InternalVisibility visibility,
    ::google::protobuf::Arena* arena)
      : _cached_size_{0},
        name_(arena) {}

inline void PinConnection::SharedCtor(::_pb::Arena* arena) {
  new (&_impl_) Impl_(internal_visibility(), arena);
  ::memset(reinterpret_cast<char *>(&_impl_) +
               offsetof(Impl_, component_),
           0,
           offsetof(Impl_, pin_) -
               offsetof(Impl_, component_) +
               sizeof(Impl_::pin_));
}
PinConnection::~PinConnection() {
  // @@protoc_insertion_point(destructor:Odb.Lib.Protobuf.ProductModel.PinConnection)
  SharedDtor(*this);
}
inline void PinConnection::SharedDtor(MessageLite& self) {
  PinConnection& this_ = static_cast<PinConnection&>(self);
  this_._internal_metadata_.Delete<::google::protobuf::UnknownFieldSet>();
  ABSL_DCHECK(this_.GetArena() == nullptr);
  this_._impl_.name_.Destroy();
  delete this_._impl_.component_;
  delete this_._impl_.pin_;
  this_._impl_.~Impl_();
}

inline void* PinConnection::PlacementNew_(const void*, void* mem,
                                        ::google::protobuf::Arena* arena) {
  return ::new (mem) PinConnection(arena);
}
constexpr auto PinConnection::InternalNewImpl_() {
  return ::google::protobuf::internal::MessageCreator::CopyInit(sizeof(PinConnection),
                                            alignof(PinConnection));
}
PROTOBUF_CONSTINIT
PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::google::protobuf::internal::ClassDataFull PinConnection::_class_data_ = {
    ::google::protobuf::internal::ClassData{
        &_PinConnection_default_instance_._instance,
        &_table_.header,
        nullptr,  // OnDemandRegisterArenaDtor
        nullptr,  // IsInitialized
        &PinConnection::MergeImpl,
        ::google::protobuf::Message::GetNewImpl<PinConnection>(),
#if defined(PROTOBUF_CUSTOM_VTABLE)
        &PinConnection::SharedDtor,
        ::google::protobuf::Message::GetClearImpl<PinConnection>(), &PinConnection::ByteSizeLong,
            &PinConnection::_InternalSerialize,
#endif  // PROTOBUF_CUSTOM_VTABLE
        PROTOBUF_FIELD_OFFSET(PinConnection, _impl_._cached_size_),
        false,
    },
    &PinConnection::kDescriptorMethods,
    &descriptor_table_pinconnection_2eproto,
    nullptr,  // tracker
};
const ::google::protobuf::internal::ClassData* PinConnection::GetClassData() const {
  ::google::protobuf::internal::PrefetchToLocalCache(&_class_data_);
  ::google::protobuf::internal::PrefetchToLocalCache(_class_data_.tc_table);
  return _class_data_.base();
}
PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1
const ::_pbi::TcParseTable<2, 3, 2, 56, 2> PinConnection::_table_ = {
  {
    PROTOBUF_FIELD_OFFSET(PinConnection, _impl_._has_bits_),
    0, // no _extensions_
    3, 24,  // max_field_number, fast_idx_mask
    offsetof(decltype(_table_), field_lookup_table),
    4294967288,  // skipmap
    offsetof(decltype(_table_), field_entries),
    3,  // num_field_entries
    2,  // num_aux_entries
    offsetof(decltype(_table_), aux_entries),
    _class_data_.base(),
    nullptr,  // post_loop_handler
    ::_pbi::TcParser::GenericFallback,  // fallback
    #ifdef PROTOBUF_PREFETCH_PARSE_TABLE
    ::_pbi::TcParser::GetTable<::Odb::Lib::Protobuf::ProductModel::PinConnection>(),  // to_prefetch
    #endif  // PROTOBUF_PREFETCH_PARSE_TABLE
  }, {{
    {::_pbi::TcParser::MiniParse, {}},
    // optional string name = 1;
    {::_pbi::TcParser::FastUS1,
     {10, 0, 0, PROTOBUF_FIELD_OFFSET(PinConnection, _impl_.name_)}},
    // optional .Odb.Lib.Protobuf.ProductModel.Component component = 2;
    {::_pbi::TcParser::FastMtS1,
     {18, 1, 0, PROTOBUF_FIELD_OFFSET(PinConnection, _impl_.component_)}},
    // optional .Odb.Lib.Protobuf.ProductModel.Pin pin = 3;
    {::_pbi::TcParser::FastMtS1,
     {26, 2, 1, PROTOBUF_FIELD_OFFSET(PinConnection, _impl_.pin_)}},
  }}, {{
    65535, 65535
  }}, {{
    // optional string name = 1;
    {PROTOBUF_FIELD_OFFSET(PinConnection, _impl_.name_), _Internal::kHasBitsOffset + 0, 0,
    (0 | ::_fl::kFcOptional | ::_fl::kUtf8String | ::_fl::kRepAString)},
    // optional .Odb.Lib.Protobuf.ProductModel.Component component = 2;
    {PROTOBUF_FIELD_OFFSET(PinConnection, _impl_.component_), _Internal::kHasBitsOffset + 1, 0,
    (0 | ::_fl::kFcOptional | ::_fl::kMessage | ::_fl::kTvTable)},
    // optional .Odb.Lib.Protobuf.ProductModel.Pin pin = 3;
    {PROTOBUF_FIELD_OFFSET(PinConnection, _impl_.pin_), _Internal::kHasBitsOffset + 2, 1,
    (0 | ::_fl::kFcOptional | ::_fl::kMessage | ::_fl::kTvTable)},
  }}, {{
    {::_pbi::TcParser::GetTable<::Odb::Lib::Protobuf::ProductModel::Component>()},
    {::_pbi::TcParser::GetTable<::Odb::Lib::Protobuf::ProductModel::Pin>()},
  }}, {{
    "\53\4\0\0\0\0\0\0"
    "Odb.Lib.Protobuf.ProductModel.PinConnection"
    "name"
  }},
};

PROTOBUF_NOINLINE void PinConnection::Clear() {
// @@protoc_insertion_point(message_clear_start:Odb.Lib.Protobuf.ProductModel.PinConnection)
  ::google::protobuf::internal::TSanWrite(&_impl_);
  ::uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _impl_._has_bits_[0];
  if (cached_has_bits & 0x00000007u) {
    if (cached_has_bits & 0x00000001u) {
      _impl_.name_.ClearNonDefaultToEmpty();
    }
    if (cached_has_bits & 0x00000002u) {
      ABSL_DCHECK(_impl_.component_ != nullptr);
      _impl_.component_->Clear();
    }
    if (cached_has_bits & 0x00000004u) {
      ABSL_DCHECK(_impl_.pin_ != nullptr);
      _impl_.pin_->Clear();
    }
  }
  _impl_._has_bits_.Clear();
  _internal_metadata_.Clear<::google::protobuf::UnknownFieldSet>();
}

#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::uint8_t* PinConnection::_InternalSerialize(
            const MessageLite& base, ::uint8_t* target,
            ::google::protobuf::io::EpsCopyOutputStream* stream) {
          const PinConnection& this_ = static_cast<const PinConnection&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::uint8_t* PinConnection::_InternalSerialize(
            ::uint8_t* target,
            ::google::protobuf::io::EpsCopyOutputStream* stream) const {
          const PinConnection& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          // @@protoc_insertion_point(serialize_to_array_start:Odb.Lib.Protobuf.ProductModel.PinConnection)
          ::uint32_t cached_has_bits = 0;
          (void)cached_has_bits;

          cached_has_bits = this_._impl_._has_bits_[0];
          // optional string name = 1;
          if (cached_has_bits & 0x00000001u) {
            const std::string& _s = this_._internal_name();
            ::google::protobuf::internal::WireFormatLite::VerifyUtf8String(
                _s.data(), static_cast<int>(_s.length()), ::google::protobuf::internal::WireFormatLite::SERIALIZE, "Odb.Lib.Protobuf.ProductModel.PinConnection.name");
            target = stream->WriteStringMaybeAliased(1, _s, target);
          }

          // optional .Odb.Lib.Protobuf.ProductModel.Component component = 2;
          if (cached_has_bits & 0x00000002u) {
            target = ::google::protobuf::internal::WireFormatLite::InternalWriteMessage(
                2, *this_._impl_.component_, this_._impl_.component_->GetCachedSize(), target,
                stream);
          }

          // optional .Odb.Lib.Protobuf.ProductModel.Pin pin = 3;
          if (cached_has_bits & 0x00000004u) {
            target = ::google::protobuf::internal::WireFormatLite::InternalWriteMessage(
                3, *this_._impl_.pin_, this_._impl_.pin_->GetCachedSize(), target,
                stream);
          }

          if (PROTOBUF_PREDICT_FALSE(this_._internal_metadata_.have_unknown_fields())) {
            target =
                ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
                    this_._internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance), target, stream);
          }
          // @@protoc_insertion_point(serialize_to_array_end:Odb.Lib.Protobuf.ProductModel.PinConnection)
          return target;
        }

#if defined(PROTOBUF_CUSTOM_VTABLE)
        ::size_t PinConnection::ByteSizeLong(const MessageLite& base) {
          const PinConnection& this_ = static_cast<const PinConnection&>(base);
#else   // PROTOBUF_CUSTOM_VTABLE
        ::size_t PinConnection::ByteSizeLong() const {
          const PinConnection& this_ = *this;
#endif  // PROTOBUF_CUSTOM_VTABLE
          // @@protoc_insertion_point(message_byte_size_start:Odb.Lib.Protobuf.ProductModel.PinConnection)
          ::size_t total_size = 0;

          ::uint32_t cached_has_bits = 0;
          // Prevent compiler warnings about cached_has_bits being unused
          (void)cached_has_bits;

          ::_pbi::Prefetch5LinesFrom7Lines(&this_);
          cached_has_bits = this_._impl_._has_bits_[0];
          if (cached_has_bits & 0x00000007u) {
            // optional string name = 1;
            if (cached_has_bits & 0x00000001u) {
              total_size += 1 + ::google::protobuf::internal::WireFormatLite::StringSize(
                                              this_._internal_name());
            }
            // optional .Odb.Lib.Protobuf.ProductModel.Component component = 2;
            if (cached_has_bits & 0x00000002u) {
              total_size += 1 +
                            ::google::protobuf::internal::WireFormatLite::MessageSize(*this_._impl_.component_);
            }
            // optional .Odb.Lib.Protobuf.ProductModel.Pin pin = 3;
            if (cached_has_bits & 0x00000004u) {
              total_size += 1 +
                            ::google::protobuf::internal::WireFormatLite::MessageSize(*this_._impl_.pin_);
            }
          }
          return this_.MaybeComputeUnknownFieldsSize(total_size,
                                                     &this_._impl_._cached_size_);
        }

void PinConnection::MergeImpl(::google::protobuf::MessageLite& to_msg, const ::google::protobuf::MessageLite& from_msg) {
  auto* const _this = static_cast<PinConnection*>(&to_msg);
  auto& from = static_cast<const PinConnection&>(from_msg);
  ::google::protobuf::Arena* arena = _this->GetArena();
  // @@protoc_insertion_point(class_specific_merge_from_start:Odb.Lib.Protobuf.ProductModel.PinConnection)
  ABSL_DCHECK_NE(&from, _this);
  ::uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = from._impl_._has_bits_[0];
  if (cached_has_bits & 0x00000007u) {
    if (cached_has_bits & 0x00000001u) {
      _this->_internal_set_name(from._internal_name());
    }
    if (cached_has_bits & 0x00000002u) {
      ABSL_DCHECK(from._impl_.component_ != nullptr);
      if (_this->_impl_.component_ == nullptr) {
        _this->_impl_.component_ =
            ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::ProductModel::Component>(arena, *from._impl_.component_);
      } else {
        _this->_impl_.component_->MergeFrom(*from._impl_.component_);
      }
    }
    if (cached_has_bits & 0x00000004u) {
      ABSL_DCHECK(from._impl_.pin_ != nullptr);
      if (_this->_impl_.pin_ == nullptr) {
        _this->_impl_.pin_ =
            ::google::protobuf::Message::CopyConstruct<::Odb::Lib::Protobuf::ProductModel::Pin>(arena, *from._impl_.pin_);
      } else {
        _this->_impl_.pin_->MergeFrom(*from._impl_.pin_);
      }
    }
  }
  _this->_impl_._has_bits_[0] |= cached_has_bits;
  _this->_internal_metadata_.MergeFrom<::google::protobuf::UnknownFieldSet>(from._internal_metadata_);
}

void PinConnection::CopyFrom(const PinConnection& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:Odb.Lib.Protobuf.ProductModel.PinConnection)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}


void PinConnection::InternalSwap(PinConnection* PROTOBUF_RESTRICT other) {
  using std::swap;
  auto* arena = GetArena();
  ABSL_DCHECK_EQ(arena, other->GetArena());
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  swap(_impl_._has_bits_[0], other->_impl_._has_bits_[0]);
  ::_pbi::ArenaStringPtr::InternalSwap(&_impl_.name_, &other->_impl_.name_, arena);
  ::google::protobuf::internal::memswap<
      PROTOBUF_FIELD_OFFSET(PinConnection, _impl_.pin_)
      + sizeof(PinConnection::_impl_.pin_)
      - PROTOBUF_FIELD_OFFSET(PinConnection, _impl_.component_)>(
          reinterpret_cast<char*>(&_impl_.component_),
          reinterpret_cast<char*>(&other->_impl_.component_));
}

::google::protobuf::Metadata PinConnection::GetMetadata() const {
  return ::google::protobuf::Message::GetMetadataImpl(GetClassData()->full());
}
// @@protoc_insertion_point(namespace_scope)
}  // namespace ProductModel
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
        (::_pbi::AddDescriptors(&descriptor_table_pinconnection_2eproto),
         ::std::false_type{});
#include "google/protobuf/port_undef.inc"
