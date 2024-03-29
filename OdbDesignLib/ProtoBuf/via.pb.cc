// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: via.proto

#include "via.pb.h"

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
namespace ProductModel {
PROTOBUF_CONSTEXPR Via::Via(
    ::_pbi::ConstantInitialized): _impl_{
    /*decltype(_impl_._has_bits_)*/{}
  , /*decltype(_impl_._cached_size_)*/{}
  , /*decltype(_impl_.name_)*/{&::_pbi::fixed_address_empty_string, ::_pbi::ConstantInitialized{}}
  , /*decltype(_impl_.boardside_)*/0} {}
struct ViaDefaultTypeInternal {
  PROTOBUF_CONSTEXPR ViaDefaultTypeInternal()
      : _instance(::_pbi::ConstantInitialized{}) {}
  ~ViaDefaultTypeInternal() {}
  union {
    Via _instance;
  };
};
PROTOBUF_ATTRIBUTE_NO_DESTROY PROTOBUF_CONSTINIT PROTOBUF_ATTRIBUTE_INIT_PRIORITY1 ViaDefaultTypeInternal _Via_default_instance_;
}  // namespace ProductModel
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
static ::_pb::Metadata file_level_metadata_via_2eproto[1];
static constexpr ::_pb::EnumDescriptor const** file_level_enum_descriptors_via_2eproto = nullptr;
static constexpr ::_pb::ServiceDescriptor const** file_level_service_descriptors_via_2eproto = nullptr;

const uint32_t TableStruct_via_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::ProductModel::Via, _impl_._has_bits_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::ProductModel::Via, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  ~0u,  // no _inlined_string_donated_
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::ProductModel::Via, _impl_.name_),
  PROTOBUF_FIELD_OFFSET(::Odb::Lib::Protobuf::ProductModel::Via, _impl_.boardside_),
  0,
  1,
};
static const ::_pbi::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, 8, -1, sizeof(::Odb::Lib::Protobuf::ProductModel::Via)},
};

static const ::_pb::Message* const file_default_instances[] = {
  &::Odb::Lib::Protobuf::ProductModel::_Via_default_instance_._instance,
};

const char descriptor_table_protodef_via_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\tvia.proto\022\035Odb.Lib.Protobuf.ProductMod"
  "el\032\013enums.proto\"d\n\003Via\022\021\n\004name\030\001 \001(\tH\000\210\001"
  "\001\0223\n\tboardSide\030\002 \001(\0162\033.Odb.Lib.Protobuf."
  "BoardSideH\001\210\001\001B\007\n\005_nameB\014\n\n_boardSideb\006p"
  "roto3"
  ;
static const ::_pbi::DescriptorTable* const descriptor_table_via_2eproto_deps[1] = {
  &::descriptor_table_enums_2eproto,
};
static ::_pbi::once_flag descriptor_table_via_2eproto_once;
const ::_pbi::DescriptorTable descriptor_table_via_2eproto = {
    false, false, 165, descriptor_table_protodef_via_2eproto,
    "via.proto",
    &descriptor_table_via_2eproto_once, descriptor_table_via_2eproto_deps, 1, 1,
    schemas, file_default_instances, TableStruct_via_2eproto::offsets,
    file_level_metadata_via_2eproto, file_level_enum_descriptors_via_2eproto,
    file_level_service_descriptors_via_2eproto,
};
PROTOBUF_ATTRIBUTE_WEAK const ::_pbi::DescriptorTable* descriptor_table_via_2eproto_getter() {
  return &descriptor_table_via_2eproto;
}

// Force running AddDescriptors() at dynamic initialization time.
PROTOBUF_ATTRIBUTE_INIT_PRIORITY2 static ::_pbi::AddDescriptorsRunner dynamic_init_dummy_via_2eproto(&descriptor_table_via_2eproto);
namespace Odb {
namespace Lib {
namespace Protobuf {
namespace ProductModel {

// ===================================================================

class Via::_Internal {
 public:
  using HasBits = decltype(std::declval<Via>()._impl_._has_bits_);
  static void set_has_name(HasBits* has_bits) {
    (*has_bits)[0] |= 1u;
  }
  static void set_has_boardside(HasBits* has_bits) {
    (*has_bits)[0] |= 2u;
  }
};

Via::Via(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                         bool is_message_owned)
  : ::PROTOBUF_NAMESPACE_ID::Message(arena, is_message_owned) {
  SharedCtor(arena, is_message_owned);
  // @@protoc_insertion_point(arena_constructor:Odb.Lib.Protobuf.ProductModel.Via)
}
Via::Via(const Via& from)
  : ::PROTOBUF_NAMESPACE_ID::Message() {
  Via* const _this = this; (void)_this;
  new (&_impl_) Impl_{
      decltype(_impl_._has_bits_){from._impl_._has_bits_}
    , /*decltype(_impl_._cached_size_)*/{}
    , decltype(_impl_.name_){}
    , decltype(_impl_.boardside_){}};

  _internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
  _impl_.name_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.name_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (from._internal_has_name()) {
    _this->_impl_.name_.Set(from._internal_name(), 
      _this->GetArenaForAllocation());
  }
  _this->_impl_.boardside_ = from._impl_.boardside_;
  // @@protoc_insertion_point(copy_constructor:Odb.Lib.Protobuf.ProductModel.Via)
}

inline void Via::SharedCtor(
    ::_pb::Arena* arena, bool is_message_owned) {
  (void)arena;
  (void)is_message_owned;
  new (&_impl_) Impl_{
      decltype(_impl_._has_bits_){}
    , /*decltype(_impl_._cached_size_)*/{}
    , decltype(_impl_.name_){}
    , decltype(_impl_.boardside_){0}
  };
  _impl_.name_.InitDefault();
  #ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
    _impl_.name_.Set("", GetArenaForAllocation());
  #endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
}

Via::~Via() {
  // @@protoc_insertion_point(destructor:Odb.Lib.Protobuf.ProductModel.Via)
  if (auto *arena = _internal_metadata_.DeleteReturnArena<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>()) {
  (void)arena;
    return;
  }
  SharedDtor();
}

inline void Via::SharedDtor() {
  GOOGLE_DCHECK(GetArenaForAllocation() == nullptr);
  _impl_.name_.Destroy();
}

void Via::SetCachedSize(int size) const {
  _impl_._cached_size_.Set(size);
}

void Via::Clear() {
// @@protoc_insertion_point(message_clear_start:Odb.Lib.Protobuf.ProductModel.Via)
  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _impl_._has_bits_[0];
  if (cached_has_bits & 0x00000001u) {
    _impl_.name_.ClearNonDefaultToEmpty();
  }
  _impl_.boardside_ = 0;
  _impl_._has_bits_.Clear();
  _internal_metadata_.Clear<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>();
}

const char* Via::_InternalParse(const char* ptr, ::_pbi::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  _Internal::HasBits has_bits{};
  while (!ctx->Done(&ptr)) {
    uint32_t tag;
    ptr = ::_pbi::ReadTag(ptr, &tag);
    switch (tag >> 3) {
      // optional string name = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 10)) {
          auto str = _internal_mutable_name();
          ptr = ::_pbi::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(ptr);
          CHK_(::_pbi::VerifyUTF8(str, "Odb.Lib.Protobuf.ProductModel.Via.name"));
        } else
          goto handle_unusual;
        continue;
      // optional .Odb.Lib.Protobuf.BoardSide boardSide = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<uint8_t>(tag) == 16)) {
          uint64_t val = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint64(&ptr);
          CHK_(ptr);
          _internal_set_boardside(static_cast<::Odb::Lib::Protobuf::BoardSide>(val));
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

uint8_t* Via::_InternalSerialize(
    uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:Odb.Lib.Protobuf.ProductModel.Via)
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  // optional string name = 1;
  if (_internal_has_name()) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_name().data(), static_cast<int>(this->_internal_name().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "Odb.Lib.Protobuf.ProductModel.Via.name");
    target = stream->WriteStringMaybeAliased(
        1, this->_internal_name(), target);
  }

  // optional .Odb.Lib.Protobuf.BoardSide boardSide = 2;
  if (_internal_has_boardside()) {
    target = stream->EnsureSpace(target);
    target = ::_pbi::WireFormatLite::WriteEnumToArray(
      2, this->_internal_boardside(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::_pbi::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(::PROTOBUF_NAMESPACE_ID::UnknownFieldSet::default_instance), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:Odb.Lib.Protobuf.ProductModel.Via)
  return target;
}

size_t Via::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:Odb.Lib.Protobuf.ProductModel.Via)
  size_t total_size = 0;

  uint32_t cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  cached_has_bits = _impl_._has_bits_[0];
  if (cached_has_bits & 0x00000003u) {
    // optional string name = 1;
    if (cached_has_bits & 0x00000001u) {
      total_size += 1 +
        ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
          this->_internal_name());
    }

    // optional .Odb.Lib.Protobuf.BoardSide boardSide = 2;
    if (cached_has_bits & 0x00000002u) {
      total_size += 1 +
        ::_pbi::WireFormatLite::EnumSize(this->_internal_boardside());
    }

  }
  return MaybeComputeUnknownFieldsSize(total_size, &_impl_._cached_size_);
}

const ::PROTOBUF_NAMESPACE_ID::Message::ClassData Via::_class_data_ = {
    ::PROTOBUF_NAMESPACE_ID::Message::CopyWithSourceCheck,
    Via::MergeImpl
};
const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*Via::GetClassData() const { return &_class_data_; }


void Via::MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg) {
  auto* const _this = static_cast<Via*>(&to_msg);
  auto& from = static_cast<const Via&>(from_msg);
  // @@protoc_insertion_point(class_specific_merge_from_start:Odb.Lib.Protobuf.ProductModel.Via)
  GOOGLE_DCHECK_NE(&from, _this);
  uint32_t cached_has_bits = 0;
  (void) cached_has_bits;

  cached_has_bits = from._impl_._has_bits_[0];
  if (cached_has_bits & 0x00000003u) {
    if (cached_has_bits & 0x00000001u) {
      _this->_internal_set_name(from._internal_name());
    }
    if (cached_has_bits & 0x00000002u) {
      _this->_impl_.boardside_ = from._impl_.boardside_;
    }
    _this->_impl_._has_bits_[0] |= cached_has_bits;
  }
  _this->_internal_metadata_.MergeFrom<::PROTOBUF_NAMESPACE_ID::UnknownFieldSet>(from._internal_metadata_);
}

void Via::CopyFrom(const Via& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:Odb.Lib.Protobuf.ProductModel.Via)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool Via::IsInitialized() const {
  return true;
}

void Via::InternalSwap(Via* other) {
  using std::swap;
  auto* lhs_arena = GetArenaForAllocation();
  auto* rhs_arena = other->GetArenaForAllocation();
  _internal_metadata_.InternalSwap(&other->_internal_metadata_);
  swap(_impl_._has_bits_[0], other->_impl_._has_bits_[0]);
  ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr::InternalSwap(
      &_impl_.name_, lhs_arena,
      &other->_impl_.name_, rhs_arena
  );
  swap(_impl_.boardside_, other->_impl_.boardside_);
}

::PROTOBUF_NAMESPACE_ID::Metadata Via::GetMetadata() const {
  return ::_pbi::AssignDescriptors(
      &descriptor_table_via_2eproto_getter, &descriptor_table_via_2eproto_once,
      file_level_metadata_via_2eproto[0]);
}

// @@protoc_insertion_point(namespace_scope)
}  // namespace ProductModel
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::Odb::Lib::Protobuf::ProductModel::Via*
Arena::CreateMaybeMessage< ::Odb::Lib::Protobuf::ProductModel::Via >(Arena* arena) {
  return Arena::CreateMessageInternal< ::Odb::Lib::Protobuf::ProductModel::Via >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>
