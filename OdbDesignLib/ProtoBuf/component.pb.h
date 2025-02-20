// Generated by the protocol buffer compiler.  DO NOT EDIT!
// NO CHECKED-IN PROTOBUF GENCODE
// source: component.proto
// Protobuf C++ Version: 5.29.2

#ifndef component_2eproto_2epb_2eh
#define component_2eproto_2epb_2eh

#include <limits>
#include <string>
#include <type_traits>
#include <utility>

#include "google/protobuf/runtime_version.h"
#if PROTOBUF_VERSION != 5029002
#error "Protobuf C++ gencode is built with an incompatible version of"
#error "Protobuf C++ headers/runtime. See"
#error "https://protobuf.dev/support/cross-version-runtime-guarantee/#cpp"
#endif
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/arena.h"
#include "google/protobuf/arenastring.h"
#include "google/protobuf/generated_message_tctable_decl.h"
#include "google/protobuf/generated_message_util.h"
#include "google/protobuf/metadata_lite.h"
#include "google/protobuf/generated_message_reflection.h"
#include "google/protobuf/message.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/repeated_field.h"  // IWYU pragma: export
#include "google/protobuf/extension_set.h"  // IWYU pragma: export
#include "google/protobuf/unknown_field_set.h"
#include "enums.pb.h"
#include "part.pb.h"
#include "package.pb.h"
// @@protoc_insertion_point(includes)

// Must be included last.
#include "google/protobuf/port_def.inc"

#define PROTOBUF_INTERNAL_EXPORT_component_2eproto ODBDESIGN_EXPORT

namespace google {
namespace protobuf {
namespace internal {
template <typename T>
::absl::string_view GetAnyMessageName();
}  // namespace internal
}  // namespace protobuf
}  // namespace google

// Internal implementation detail -- do not use these members.
struct ODBDESIGN_EXPORT TableStruct_component_2eproto {
  static const ::uint32_t offsets[];
};
ODBDESIGN_EXPORT extern const ::google::protobuf::internal::DescriptorTable
    descriptor_table_component_2eproto;
namespace Odb {
namespace Lib {
namespace Protobuf {
namespace ProductModel {
class Component;
struct ComponentDefaultTypeInternal;
ODBDESIGN_EXPORT extern ComponentDefaultTypeInternal _Component_default_instance_;
}  // namespace ProductModel
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
namespace google {
namespace protobuf {
}  // namespace protobuf
}  // namespace google

namespace Odb {
namespace Lib {
namespace Protobuf {
namespace ProductModel {

// ===================================================================


// -------------------------------------------------------------------

class ODBDESIGN_EXPORT Component final : public ::google::protobuf::Message
/* @@protoc_insertion_point(class_definition:Odb.Lib.Protobuf.ProductModel.Component) */ {
 public:
  inline Component() : Component(nullptr) {}
  ~Component() PROTOBUF_FINAL;

#if defined(PROTOBUF_CUSTOM_VTABLE)
  void operator delete(Component* msg, std::destroying_delete_t) {
    SharedDtor(*msg);
    ::google::protobuf::internal::SizedDelete(msg, sizeof(Component));
  }
#endif

  template <typename = void>
  explicit PROTOBUF_CONSTEXPR Component(
      ::google::protobuf::internal::ConstantInitialized);

  inline Component(const Component& from) : Component(nullptr, from) {}
  inline Component(Component&& from) noexcept
      : Component(nullptr, std::move(from)) {}
  inline Component& operator=(const Component& from) {
    CopyFrom(from);
    return *this;
  }
  inline Component& operator=(Component&& from) noexcept {
    if (this == &from) return *this;
    if (::google::protobuf::internal::CanMoveWithInternalSwap(GetArena(), from.GetArena())) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.unknown_fields<::google::protobuf::UnknownFieldSet>(::google::protobuf::UnknownFieldSet::default_instance);
  }
  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return _internal_metadata_.mutable_unknown_fields<::google::protobuf::UnknownFieldSet>();
  }

  static const ::google::protobuf::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::google::protobuf::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::google::protobuf::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const Component& default_instance() {
    return *internal_default_instance();
  }
  static inline const Component* internal_default_instance() {
    return reinterpret_cast<const Component*>(
        &_Component_default_instance_);
  }
  static constexpr int kIndexInFileMessages = 0;
  friend void swap(Component& a, Component& b) { a.Swap(&b); }
  inline void Swap(Component* other) {
    if (other == this) return;
    if (::google::protobuf::internal::CanUseInternalSwap(GetArena(), other->GetArena())) {
      InternalSwap(other);
    } else {
      ::google::protobuf::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Component* other) {
    if (other == this) return;
    ABSL_DCHECK(GetArena() == other->GetArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  Component* New(::google::protobuf::Arena* arena = nullptr) const {
    return ::google::protobuf::Message::DefaultConstruct<Component>(arena);
  }
  using ::google::protobuf::Message::CopyFrom;
  void CopyFrom(const Component& from);
  using ::google::protobuf::Message::MergeFrom;
  void MergeFrom(const Component& from) { Component::MergeImpl(*this, from); }

  private:
  static void MergeImpl(
      ::google::protobuf::MessageLite& to_msg,
      const ::google::protobuf::MessageLite& from_msg);

  public:
  bool IsInitialized() const {
    return true;
  }
  ABSL_ATTRIBUTE_REINITIALIZES void Clear() PROTOBUF_FINAL;
  #if defined(PROTOBUF_CUSTOM_VTABLE)
  private:
  static ::size_t ByteSizeLong(const ::google::protobuf::MessageLite& msg);
  static ::uint8_t* _InternalSerialize(
      const MessageLite& msg, ::uint8_t* target,
      ::google::protobuf::io::EpsCopyOutputStream* stream);

  public:
  ::size_t ByteSizeLong() const { return ByteSizeLong(*this); }
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target,
      ::google::protobuf::io::EpsCopyOutputStream* stream) const {
    return _InternalSerialize(*this, target, stream);
  }
  #else   // PROTOBUF_CUSTOM_VTABLE
  ::size_t ByteSizeLong() const final;
  ::uint8_t* _InternalSerialize(
      ::uint8_t* target,
      ::google::protobuf::io::EpsCopyOutputStream* stream) const final;
  #endif  // PROTOBUF_CUSTOM_VTABLE
  int GetCachedSize() const { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::google::protobuf::Arena* arena);
  static void SharedDtor(MessageLite& self);
  void InternalSwap(Component* other);
 private:
  template <typename T>
  friend ::absl::string_view(
      ::google::protobuf::internal::GetAnyMessageName)();
  static ::absl::string_view FullMessageName() { return "Odb.Lib.Protobuf.ProductModel.Component"; }

 protected:
  explicit Component(::google::protobuf::Arena* arena);
  Component(::google::protobuf::Arena* arena, const Component& from);
  Component(::google::protobuf::Arena* arena, Component&& from) noexcept
      : Component(arena) {
    *this = ::std::move(from);
  }
  const ::google::protobuf::internal::ClassData* GetClassData() const PROTOBUF_FINAL;
  static void* PlacementNew_(const void*, void* mem,
                             ::google::protobuf::Arena* arena);
  static constexpr auto InternalNewImpl_();
  static const ::google::protobuf::internal::ClassDataFull _class_data_;

 public:
  ::google::protobuf::Metadata GetMetadata() const;
  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------
  enum : int {
    kRefDesFieldNumber = 1,
    kPartNameFieldNumber = 2,
    kPackageFieldNumber = 3,
    kPartFieldNumber = 6,
    kIndexFieldNumber = 4,
    kSideFieldNumber = 5,
  };
  // optional string refDes = 1;
  bool has_refdes() const;
  void clear_refdes() ;
  const std::string& refdes() const;
  template <typename Arg_ = const std::string&, typename... Args_>
  void set_refdes(Arg_&& arg, Args_... args);
  std::string* mutable_refdes();
  PROTOBUF_NODISCARD std::string* release_refdes();
  void set_allocated_refdes(std::string* value);

  private:
  const std::string& _internal_refdes() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_refdes(
      const std::string& value);
  std::string* _internal_mutable_refdes();

  public:
  // optional string partName = 2;
  bool has_partname() const;
  void clear_partname() ;
  const std::string& partname() const;
  template <typename Arg_ = const std::string&, typename... Args_>
  void set_partname(Arg_&& arg, Args_... args);
  std::string* mutable_partname();
  PROTOBUF_NODISCARD std::string* release_partname();
  void set_allocated_partname(std::string* value);

  private:
  const std::string& _internal_partname() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_partname(
      const std::string& value);
  std::string* _internal_mutable_partname();

  public:
  // optional .Odb.Lib.Protobuf.ProductModel.Package package = 3;
  bool has_package() const;
  void clear_package() ;
  const ::Odb::Lib::Protobuf::ProductModel::Package& package() const;
  PROTOBUF_NODISCARD ::Odb::Lib::Protobuf::ProductModel::Package* release_package();
  ::Odb::Lib::Protobuf::ProductModel::Package* mutable_package();
  void set_allocated_package(::Odb::Lib::Protobuf::ProductModel::Package* value);
  void unsafe_arena_set_allocated_package(::Odb::Lib::Protobuf::ProductModel::Package* value);
  ::Odb::Lib::Protobuf::ProductModel::Package* unsafe_arena_release_package();

  private:
  const ::Odb::Lib::Protobuf::ProductModel::Package& _internal_package() const;
  ::Odb::Lib::Protobuf::ProductModel::Package* _internal_mutable_package();

  public:
  // optional .Odb.Lib.Protobuf.ProductModel.Part part = 6;
  bool has_part() const;
  void clear_part() ;
  const ::Odb::Lib::Protobuf::ProductModel::Part& part() const;
  PROTOBUF_NODISCARD ::Odb::Lib::Protobuf::ProductModel::Part* release_part();
  ::Odb::Lib::Protobuf::ProductModel::Part* mutable_part();
  void set_allocated_part(::Odb::Lib::Protobuf::ProductModel::Part* value);
  void unsafe_arena_set_allocated_part(::Odb::Lib::Protobuf::ProductModel::Part* value);
  ::Odb::Lib::Protobuf::ProductModel::Part* unsafe_arena_release_part();

  private:
  const ::Odb::Lib::Protobuf::ProductModel::Part& _internal_part() const;
  ::Odb::Lib::Protobuf::ProductModel::Part* _internal_mutable_part();

  public:
  // optional uint32 index = 4;
  bool has_index() const;
  void clear_index() ;
  ::uint32_t index() const;
  void set_index(::uint32_t value);

  private:
  ::uint32_t _internal_index() const;
  void _internal_set_index(::uint32_t value);

  public:
  // optional .Odb.Lib.Protobuf.BoardSide side = 5;
  bool has_side() const;
  void clear_side() ;
  ::Odb::Lib::Protobuf::BoardSide side() const;
  void set_side(::Odb::Lib::Protobuf::BoardSide value);

  private:
  ::Odb::Lib::Protobuf::BoardSide _internal_side() const;
  void _internal_set_side(::Odb::Lib::Protobuf::BoardSide value);

  public:
  // @@protoc_insertion_point(class_scope:Odb.Lib.Protobuf.ProductModel.Component)
 private:
  class _Internal;
  friend class ::google::protobuf::internal::TcParser;
  static const ::google::protobuf::internal::TcParseTable<
      3, 6, 2,
      62, 2>
      _table_;

  friend class ::google::protobuf::MessageLite;
  friend class ::google::protobuf::Arena;
  template <typename T>
  friend class ::google::protobuf::Arena::InternalHelper;
  using InternalArenaConstructable_ = void;
  using DestructorSkippable_ = void;
  struct Impl_ {
    inline explicit constexpr Impl_(
        ::google::protobuf::internal::ConstantInitialized) noexcept;
    inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                          ::google::protobuf::Arena* arena);
    inline explicit Impl_(::google::protobuf::internal::InternalVisibility visibility,
                          ::google::protobuf::Arena* arena, const Impl_& from,
                          const Component& from_msg);
    ::google::protobuf::internal::HasBits<1> _has_bits_;
    ::google::protobuf::internal::CachedSize _cached_size_;
    ::google::protobuf::internal::ArenaStringPtr refdes_;
    ::google::protobuf::internal::ArenaStringPtr partname_;
    ::Odb::Lib::Protobuf::ProductModel::Package* package_;
    ::Odb::Lib::Protobuf::ProductModel::Part* part_;
    ::uint32_t index_;
    int side_;
    PROTOBUF_TSAN_DECLARE_MEMBER
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_component_2eproto;
};

// ===================================================================




// ===================================================================


#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// -------------------------------------------------------------------

// Component

// optional string refDes = 1;
inline bool Component::has_refdes() const {
  bool value = (_impl_._has_bits_[0] & 0x00000001u) != 0;
  return value;
}
inline void Component::clear_refdes() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.refdes_.ClearToEmpty();
  _impl_._has_bits_[0] &= ~0x00000001u;
}
inline const std::string& Component::refdes() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:Odb.Lib.Protobuf.ProductModel.Component.refDes)
  return _internal_refdes();
}
template <typename Arg_, typename... Args_>
inline PROTOBUF_ALWAYS_INLINE void Component::set_refdes(Arg_&& arg,
                                                     Args_... args) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_._has_bits_[0] |= 0x00000001u;
  _impl_.refdes_.Set(static_cast<Arg_&&>(arg), args..., GetArena());
  // @@protoc_insertion_point(field_set:Odb.Lib.Protobuf.ProductModel.Component.refDes)
}
inline std::string* Component::mutable_refdes() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  std::string* _s = _internal_mutable_refdes();
  // @@protoc_insertion_point(field_mutable:Odb.Lib.Protobuf.ProductModel.Component.refDes)
  return _s;
}
inline const std::string& Component::_internal_refdes() const {
  ::google::protobuf::internal::TSanRead(&_impl_);
  return _impl_.refdes_.Get();
}
inline void Component::_internal_set_refdes(const std::string& value) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_._has_bits_[0] |= 0x00000001u;
  _impl_.refdes_.Set(value, GetArena());
}
inline std::string* Component::_internal_mutable_refdes() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_._has_bits_[0] |= 0x00000001u;
  return _impl_.refdes_.Mutable( GetArena());
}
inline std::string* Component::release_refdes() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  // @@protoc_insertion_point(field_release:Odb.Lib.Protobuf.ProductModel.Component.refDes)
  if ((_impl_._has_bits_[0] & 0x00000001u) == 0) {
    return nullptr;
  }
  _impl_._has_bits_[0] &= ~0x00000001u;
  auto* released = _impl_.refdes_.Release();
  if (::google::protobuf::internal::DebugHardenForceCopyDefaultString()) {
    _impl_.refdes_.Set("", GetArena());
  }
  return released;
}
inline void Component::set_allocated_refdes(std::string* value) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (value != nullptr) {
    _impl_._has_bits_[0] |= 0x00000001u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000001u;
  }
  _impl_.refdes_.SetAllocated(value, GetArena());
  if (::google::protobuf::internal::DebugHardenForceCopyDefaultString() && _impl_.refdes_.IsDefault()) {
    _impl_.refdes_.Set("", GetArena());
  }
  // @@protoc_insertion_point(field_set_allocated:Odb.Lib.Protobuf.ProductModel.Component.refDes)
}

// optional string partName = 2;
inline bool Component::has_partname() const {
  bool value = (_impl_._has_bits_[0] & 0x00000002u) != 0;
  return value;
}
inline void Component::clear_partname() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.partname_.ClearToEmpty();
  _impl_._has_bits_[0] &= ~0x00000002u;
}
inline const std::string& Component::partname() const
    ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:Odb.Lib.Protobuf.ProductModel.Component.partName)
  return _internal_partname();
}
template <typename Arg_, typename... Args_>
inline PROTOBUF_ALWAYS_INLINE void Component::set_partname(Arg_&& arg,
                                                     Args_... args) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_._has_bits_[0] |= 0x00000002u;
  _impl_.partname_.Set(static_cast<Arg_&&>(arg), args..., GetArena());
  // @@protoc_insertion_point(field_set:Odb.Lib.Protobuf.ProductModel.Component.partName)
}
inline std::string* Component::mutable_partname() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  std::string* _s = _internal_mutable_partname();
  // @@protoc_insertion_point(field_mutable:Odb.Lib.Protobuf.ProductModel.Component.partName)
  return _s;
}
inline const std::string& Component::_internal_partname() const {
  ::google::protobuf::internal::TSanRead(&_impl_);
  return _impl_.partname_.Get();
}
inline void Component::_internal_set_partname(const std::string& value) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_._has_bits_[0] |= 0x00000002u;
  _impl_.partname_.Set(value, GetArena());
}
inline std::string* Component::_internal_mutable_partname() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_._has_bits_[0] |= 0x00000002u;
  return _impl_.partname_.Mutable( GetArena());
}
inline std::string* Component::release_partname() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  // @@protoc_insertion_point(field_release:Odb.Lib.Protobuf.ProductModel.Component.partName)
  if ((_impl_._has_bits_[0] & 0x00000002u) == 0) {
    return nullptr;
  }
  _impl_._has_bits_[0] &= ~0x00000002u;
  auto* released = _impl_.partname_.Release();
  if (::google::protobuf::internal::DebugHardenForceCopyDefaultString()) {
    _impl_.partname_.Set("", GetArena());
  }
  return released;
}
inline void Component::set_allocated_partname(std::string* value) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (value != nullptr) {
    _impl_._has_bits_[0] |= 0x00000002u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000002u;
  }
  _impl_.partname_.SetAllocated(value, GetArena());
  if (::google::protobuf::internal::DebugHardenForceCopyDefaultString() && _impl_.partname_.IsDefault()) {
    _impl_.partname_.Set("", GetArena());
  }
  // @@protoc_insertion_point(field_set_allocated:Odb.Lib.Protobuf.ProductModel.Component.partName)
}

// optional .Odb.Lib.Protobuf.ProductModel.Package package = 3;
inline bool Component::has_package() const {
  bool value = (_impl_._has_bits_[0] & 0x00000004u) != 0;
  PROTOBUF_ASSUME(!value || _impl_.package_ != nullptr);
  return value;
}
inline const ::Odb::Lib::Protobuf::ProductModel::Package& Component::_internal_package() const {
  ::google::protobuf::internal::TSanRead(&_impl_);
  const ::Odb::Lib::Protobuf::ProductModel::Package* p = _impl_.package_;
  return p != nullptr ? *p : reinterpret_cast<const ::Odb::Lib::Protobuf::ProductModel::Package&>(::Odb::Lib::Protobuf::ProductModel::_Package_default_instance_);
}
inline const ::Odb::Lib::Protobuf::ProductModel::Package& Component::package() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:Odb.Lib.Protobuf.ProductModel.Component.package)
  return _internal_package();
}
inline void Component::unsafe_arena_set_allocated_package(::Odb::Lib::Protobuf::ProductModel::Package* value) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (GetArena() == nullptr) {
    delete reinterpret_cast<::google::protobuf::MessageLite*>(_impl_.package_);
  }
  _impl_.package_ = reinterpret_cast<::Odb::Lib::Protobuf::ProductModel::Package*>(value);
  if (value != nullptr) {
    _impl_._has_bits_[0] |= 0x00000004u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000004u;
  }
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:Odb.Lib.Protobuf.ProductModel.Component.package)
}
inline ::Odb::Lib::Protobuf::ProductModel::Package* Component::release_package() {
  ::google::protobuf::internal::TSanWrite(&_impl_);

  _impl_._has_bits_[0] &= ~0x00000004u;
  ::Odb::Lib::Protobuf::ProductModel::Package* released = _impl_.package_;
  _impl_.package_ = nullptr;
  if (::google::protobuf::internal::DebugHardenForceCopyInRelease()) {
    auto* old = reinterpret_cast<::google::protobuf::MessageLite*>(released);
    released = ::google::protobuf::internal::DuplicateIfNonNull(released);
    if (GetArena() == nullptr) {
      delete old;
    }
  } else {
    if (GetArena() != nullptr) {
      released = ::google::protobuf::internal::DuplicateIfNonNull(released);
    }
  }
  return released;
}
inline ::Odb::Lib::Protobuf::ProductModel::Package* Component::unsafe_arena_release_package() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  // @@protoc_insertion_point(field_release:Odb.Lib.Protobuf.ProductModel.Component.package)

  _impl_._has_bits_[0] &= ~0x00000004u;
  ::Odb::Lib::Protobuf::ProductModel::Package* temp = _impl_.package_;
  _impl_.package_ = nullptr;
  return temp;
}
inline ::Odb::Lib::Protobuf::ProductModel::Package* Component::_internal_mutable_package() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (_impl_.package_ == nullptr) {
    auto* p = ::google::protobuf::Message::DefaultConstruct<::Odb::Lib::Protobuf::ProductModel::Package>(GetArena());
    _impl_.package_ = reinterpret_cast<::Odb::Lib::Protobuf::ProductModel::Package*>(p);
  }
  return _impl_.package_;
}
inline ::Odb::Lib::Protobuf::ProductModel::Package* Component::mutable_package() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  _impl_._has_bits_[0] |= 0x00000004u;
  ::Odb::Lib::Protobuf::ProductModel::Package* _msg = _internal_mutable_package();
  // @@protoc_insertion_point(field_mutable:Odb.Lib.Protobuf.ProductModel.Component.package)
  return _msg;
}
inline void Component::set_allocated_package(::Odb::Lib::Protobuf::ProductModel::Package* value) {
  ::google::protobuf::Arena* message_arena = GetArena();
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (message_arena == nullptr) {
    delete reinterpret_cast<::google::protobuf::MessageLite*>(_impl_.package_);
  }

  if (value != nullptr) {
    ::google::protobuf::Arena* submessage_arena = reinterpret_cast<::google::protobuf::MessageLite*>(value)->GetArena();
    if (message_arena != submessage_arena) {
      value = ::google::protobuf::internal::GetOwnedMessage(message_arena, value, submessage_arena);
    }
    _impl_._has_bits_[0] |= 0x00000004u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000004u;
  }

  _impl_.package_ = reinterpret_cast<::Odb::Lib::Protobuf::ProductModel::Package*>(value);
  // @@protoc_insertion_point(field_set_allocated:Odb.Lib.Protobuf.ProductModel.Component.package)
}

// optional uint32 index = 4;
inline bool Component::has_index() const {
  bool value = (_impl_._has_bits_[0] & 0x00000010u) != 0;
  return value;
}
inline void Component::clear_index() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.index_ = 0u;
  _impl_._has_bits_[0] &= ~0x00000010u;
}
inline ::uint32_t Component::index() const {
  // @@protoc_insertion_point(field_get:Odb.Lib.Protobuf.ProductModel.Component.index)
  return _internal_index();
}
inline void Component::set_index(::uint32_t value) {
  _internal_set_index(value);
  _impl_._has_bits_[0] |= 0x00000010u;
  // @@protoc_insertion_point(field_set:Odb.Lib.Protobuf.ProductModel.Component.index)
}
inline ::uint32_t Component::_internal_index() const {
  ::google::protobuf::internal::TSanRead(&_impl_);
  return _impl_.index_;
}
inline void Component::_internal_set_index(::uint32_t value) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.index_ = value;
}

// optional .Odb.Lib.Protobuf.BoardSide side = 5;
inline bool Component::has_side() const {
  bool value = (_impl_._has_bits_[0] & 0x00000020u) != 0;
  return value;
}
inline void Component::clear_side() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.side_ = 0;
  _impl_._has_bits_[0] &= ~0x00000020u;
}
inline ::Odb::Lib::Protobuf::BoardSide Component::side() const {
  // @@protoc_insertion_point(field_get:Odb.Lib.Protobuf.ProductModel.Component.side)
  return _internal_side();
}
inline void Component::set_side(::Odb::Lib::Protobuf::BoardSide value) {
  _internal_set_side(value);
  _impl_._has_bits_[0] |= 0x00000020u;
  // @@protoc_insertion_point(field_set:Odb.Lib.Protobuf.ProductModel.Component.side)
}
inline ::Odb::Lib::Protobuf::BoardSide Component::_internal_side() const {
  ::google::protobuf::internal::TSanRead(&_impl_);
  return static_cast<::Odb::Lib::Protobuf::BoardSide>(_impl_.side_);
}
inline void Component::_internal_set_side(::Odb::Lib::Protobuf::BoardSide value) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  _impl_.side_ = value;
}

// optional .Odb.Lib.Protobuf.ProductModel.Part part = 6;
inline bool Component::has_part() const {
  bool value = (_impl_._has_bits_[0] & 0x00000008u) != 0;
  PROTOBUF_ASSUME(!value || _impl_.part_ != nullptr);
  return value;
}
inline const ::Odb::Lib::Protobuf::ProductModel::Part& Component::_internal_part() const {
  ::google::protobuf::internal::TSanRead(&_impl_);
  const ::Odb::Lib::Protobuf::ProductModel::Part* p = _impl_.part_;
  return p != nullptr ? *p : reinterpret_cast<const ::Odb::Lib::Protobuf::ProductModel::Part&>(::Odb::Lib::Protobuf::ProductModel::_Part_default_instance_);
}
inline const ::Odb::Lib::Protobuf::ProductModel::Part& Component::part() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
  // @@protoc_insertion_point(field_get:Odb.Lib.Protobuf.ProductModel.Component.part)
  return _internal_part();
}
inline void Component::unsafe_arena_set_allocated_part(::Odb::Lib::Protobuf::ProductModel::Part* value) {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (GetArena() == nullptr) {
    delete reinterpret_cast<::google::protobuf::MessageLite*>(_impl_.part_);
  }
  _impl_.part_ = reinterpret_cast<::Odb::Lib::Protobuf::ProductModel::Part*>(value);
  if (value != nullptr) {
    _impl_._has_bits_[0] |= 0x00000008u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000008u;
  }
  // @@protoc_insertion_point(field_unsafe_arena_set_allocated:Odb.Lib.Protobuf.ProductModel.Component.part)
}
inline ::Odb::Lib::Protobuf::ProductModel::Part* Component::release_part() {
  ::google::protobuf::internal::TSanWrite(&_impl_);

  _impl_._has_bits_[0] &= ~0x00000008u;
  ::Odb::Lib::Protobuf::ProductModel::Part* released = _impl_.part_;
  _impl_.part_ = nullptr;
  if (::google::protobuf::internal::DebugHardenForceCopyInRelease()) {
    auto* old = reinterpret_cast<::google::protobuf::MessageLite*>(released);
    released = ::google::protobuf::internal::DuplicateIfNonNull(released);
    if (GetArena() == nullptr) {
      delete old;
    }
  } else {
    if (GetArena() != nullptr) {
      released = ::google::protobuf::internal::DuplicateIfNonNull(released);
    }
  }
  return released;
}
inline ::Odb::Lib::Protobuf::ProductModel::Part* Component::unsafe_arena_release_part() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  // @@protoc_insertion_point(field_release:Odb.Lib.Protobuf.ProductModel.Component.part)

  _impl_._has_bits_[0] &= ~0x00000008u;
  ::Odb::Lib::Protobuf::ProductModel::Part* temp = _impl_.part_;
  _impl_.part_ = nullptr;
  return temp;
}
inline ::Odb::Lib::Protobuf::ProductModel::Part* Component::_internal_mutable_part() {
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (_impl_.part_ == nullptr) {
    auto* p = ::google::protobuf::Message::DefaultConstruct<::Odb::Lib::Protobuf::ProductModel::Part>(GetArena());
    _impl_.part_ = reinterpret_cast<::Odb::Lib::Protobuf::ProductModel::Part*>(p);
  }
  return _impl_.part_;
}
inline ::Odb::Lib::Protobuf::ProductModel::Part* Component::mutable_part() ABSL_ATTRIBUTE_LIFETIME_BOUND {
  _impl_._has_bits_[0] |= 0x00000008u;
  ::Odb::Lib::Protobuf::ProductModel::Part* _msg = _internal_mutable_part();
  // @@protoc_insertion_point(field_mutable:Odb.Lib.Protobuf.ProductModel.Component.part)
  return _msg;
}
inline void Component::set_allocated_part(::Odb::Lib::Protobuf::ProductModel::Part* value) {
  ::google::protobuf::Arena* message_arena = GetArena();
  ::google::protobuf::internal::TSanWrite(&_impl_);
  if (message_arena == nullptr) {
    delete reinterpret_cast<::google::protobuf::MessageLite*>(_impl_.part_);
  }

  if (value != nullptr) {
    ::google::protobuf::Arena* submessage_arena = reinterpret_cast<::google::protobuf::MessageLite*>(value)->GetArena();
    if (message_arena != submessage_arena) {
      value = ::google::protobuf::internal::GetOwnedMessage(message_arena, value, submessage_arena);
    }
    _impl_._has_bits_[0] |= 0x00000008u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000008u;
  }

  _impl_.part_ = reinterpret_cast<::Odb::Lib::Protobuf::ProductModel::Part*>(value);
  // @@protoc_insertion_point(field_set_allocated:Odb.Lib.Protobuf.ProductModel.Component.part)
}

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif  // __GNUC__

// @@protoc_insertion_point(namespace_scope)
}  // namespace ProductModel
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb


// @@protoc_insertion_point(global_scope)

#include "google/protobuf/port_undef.inc"

#endif  // component_2eproto_2epb_2eh
