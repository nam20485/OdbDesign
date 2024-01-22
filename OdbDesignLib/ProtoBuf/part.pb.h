// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: part.proto

#ifndef GOOGLE_PROTOBUF_INCLUDED_part_2eproto
#define GOOGLE_PROTOBUF_INCLUDED_part_2eproto

#include <limits>
#include <string>

#include <google/protobuf/port_def.inc>
#if PROTOBUF_VERSION < 3021000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers. Please update
#error your headers.
#endif
#if 3021012 < PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers. Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/port_undef.inc>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/arena.h>
#include <google/protobuf/arenastring.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/metadata_lite.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>  // IWYU pragma: export
#include <google/protobuf/extension_set.h>  // IWYU pragma: export
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
#define PROTOBUF_INTERNAL_EXPORT_part_2eproto
PROTOBUF_NAMESPACE_OPEN
namespace internal {
class AnyMetadata;
}  // namespace internal
PROTOBUF_NAMESPACE_CLOSE

// Internal implementation detail -- do not use these members.
struct TableStruct_part_2eproto {
  static const uint32_t offsets[];
};
extern const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_part_2eproto;
namespace Odb {
namespace Lib {
namespace Protobuf {
namespace ProductModel {
class Part;
struct PartDefaultTypeInternal;
extern PartDefaultTypeInternal _Part_default_instance_;
}  // namespace ProductModel
}  // namespace Protobuf
}  // namespace Lib
}  // namespace Odb
PROTOBUF_NAMESPACE_OPEN
template<> ::Odb::Lib::Protobuf::ProductModel::Part* Arena::CreateMaybeMessage<::Odb::Lib::Protobuf::ProductModel::Part>(Arena*);
PROTOBUF_NAMESPACE_CLOSE
namespace Odb {
namespace Lib {
namespace Protobuf {
namespace ProductModel {

// ===================================================================

class Part final :
    public ::PROTOBUF_NAMESPACE_ID::Message /* @@protoc_insertion_point(class_definition:Odb.Lib.Protobuf.ProductModel.Part) */ {
 public:
  inline Part() : Part(nullptr) {}
  ~Part() override;
  explicit PROTOBUF_CONSTEXPR Part(::PROTOBUF_NAMESPACE_ID::internal::ConstantInitialized);

  Part(const Part& from);
  Part(Part&& from) noexcept
    : Part() {
    *this = ::std::move(from);
  }

  inline Part& operator=(const Part& from) {
    CopyFrom(from);
    return *this;
  }
  inline Part& operator=(Part&& from) noexcept {
    if (this == &from) return *this;
    if (GetOwningArena() == from.GetOwningArena()
  #ifdef PROTOBUF_FORCE_COPY_IN_MOVE
        && GetOwningArena() != nullptr
  #endif  // !PROTOBUF_FORCE_COPY_IN_MOVE
    ) {
      InternalSwap(&from);
    } else {
      CopyFrom(from);
    }
    return *this;
  }

  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* descriptor() {
    return GetDescriptor();
  }
  static const ::PROTOBUF_NAMESPACE_ID::Descriptor* GetDescriptor() {
    return default_instance().GetMetadata().descriptor;
  }
  static const ::PROTOBUF_NAMESPACE_ID::Reflection* GetReflection() {
    return default_instance().GetMetadata().reflection;
  }
  static const Part& default_instance() {
    return *internal_default_instance();
  }
  static inline const Part* internal_default_instance() {
    return reinterpret_cast<const Part*>(
               &_Part_default_instance_);
  }
  static constexpr int kIndexInFileMessages =
    0;

  friend void swap(Part& a, Part& b) {
    a.Swap(&b);
  }
  inline void Swap(Part* other) {
    if (other == this) return;
  #ifdef PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() != nullptr &&
        GetOwningArena() == other->GetOwningArena()) {
   #else  // PROTOBUF_FORCE_COPY_IN_SWAP
    if (GetOwningArena() == other->GetOwningArena()) {
  #endif  // !PROTOBUF_FORCE_COPY_IN_SWAP
      InternalSwap(other);
    } else {
      ::PROTOBUF_NAMESPACE_ID::internal::GenericSwap(this, other);
    }
  }
  void UnsafeArenaSwap(Part* other) {
    if (other == this) return;
    GOOGLE_DCHECK(GetOwningArena() == other->GetOwningArena());
    InternalSwap(other);
  }

  // implements Message ----------------------------------------------

  Part* New(::PROTOBUF_NAMESPACE_ID::Arena* arena = nullptr) const final {
    return CreateMaybeMessage<Part>(arena);
  }
  using ::PROTOBUF_NAMESPACE_ID::Message::CopyFrom;
  void CopyFrom(const Part& from);
  using ::PROTOBUF_NAMESPACE_ID::Message::MergeFrom;
  void MergeFrom( const Part& from) {
    Part::MergeImpl(*this, from);
  }
  private:
  static void MergeImpl(::PROTOBUF_NAMESPACE_ID::Message& to_msg, const ::PROTOBUF_NAMESPACE_ID::Message& from_msg);
  public:
  PROTOBUF_ATTRIBUTE_REINITIALIZES void Clear() final;
  bool IsInitialized() const final;

  size_t ByteSizeLong() const final;
  const char* _InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) final;
  uint8_t* _InternalSerialize(
      uint8_t* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const final;
  int GetCachedSize() const final { return _impl_._cached_size_.Get(); }

  private:
  void SharedCtor(::PROTOBUF_NAMESPACE_ID::Arena* arena, bool is_message_owned);
  void SharedDtor();
  void SetCachedSize(int size) const final;
  void InternalSwap(Part* other);

  private:
  friend class ::PROTOBUF_NAMESPACE_ID::internal::AnyMetadata;
  static ::PROTOBUF_NAMESPACE_ID::StringPiece FullMessageName() {
    return "Odb.Lib.Protobuf.ProductModel.Part";
  }
  protected:
  explicit Part(::PROTOBUF_NAMESPACE_ID::Arena* arena,
                       bool is_message_owned = false);
  public:

  static const ClassData _class_data_;
  const ::PROTOBUF_NAMESPACE_ID::Message::ClassData*GetClassData() const final;

  ::PROTOBUF_NAMESPACE_ID::Metadata GetMetadata() const final;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  enum : int {
    kNameFieldNumber = 1,
  };
  // optional string name = 1;
  bool has_name() const;
  private:
  bool _internal_has_name() const;
  public:
  void clear_name();
  const std::string& name() const;
  template <typename ArgT0 = const std::string&, typename... ArgT>
  void set_name(ArgT0&& arg0, ArgT... args);
  std::string* mutable_name();
  PROTOBUF_NODISCARD std::string* release_name();
  void set_allocated_name(std::string* name);
  private:
  const std::string& _internal_name() const;
  inline PROTOBUF_ALWAYS_INLINE void _internal_set_name(const std::string& value);
  std::string* _internal_mutable_name();
  public:

  // @@protoc_insertion_point(class_scope:Odb.Lib.Protobuf.ProductModel.Part)
 private:
  class _Internal;

  template <typename T> friend class ::PROTOBUF_NAMESPACE_ID::Arena::InternalHelper;
  typedef void InternalArenaConstructable_;
  typedef void DestructorSkippable_;
  struct Impl_ {
    ::PROTOBUF_NAMESPACE_ID::internal::HasBits<1> _has_bits_;
    mutable ::PROTOBUF_NAMESPACE_ID::internal::CachedSize _cached_size_;
    ::PROTOBUF_NAMESPACE_ID::internal::ArenaStringPtr name_;
  };
  union { Impl_ _impl_; };
  friend struct ::TableStruct_part_2eproto;
};
// ===================================================================


// ===================================================================

#ifdef __GNUC__
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif  // __GNUC__
// Part

// optional string name = 1;
inline bool Part::_internal_has_name() const {
  bool value = (_impl_._has_bits_[0] & 0x00000001u) != 0;
  return value;
}
inline bool Part::has_name() const {
  return _internal_has_name();
}
inline void Part::clear_name() {
  _impl_.name_.ClearToEmpty();
  _impl_._has_bits_[0] &= ~0x00000001u;
}
inline const std::string& Part::name() const {
  // @@protoc_insertion_point(field_get:Odb.Lib.Protobuf.ProductModel.Part.name)
  return _internal_name();
}
template <typename ArgT0, typename... ArgT>
inline PROTOBUF_ALWAYS_INLINE
void Part::set_name(ArgT0&& arg0, ArgT... args) {
 _impl_._has_bits_[0] |= 0x00000001u;
 _impl_.name_.Set(static_cast<ArgT0 &&>(arg0), args..., GetArenaForAllocation());
  // @@protoc_insertion_point(field_set:Odb.Lib.Protobuf.ProductModel.Part.name)
}
inline std::string* Part::mutable_name() {
  std::string* _s = _internal_mutable_name();
  // @@protoc_insertion_point(field_mutable:Odb.Lib.Protobuf.ProductModel.Part.name)
  return _s;
}
inline const std::string& Part::_internal_name() const {
  return _impl_.name_.Get();
}
inline void Part::_internal_set_name(const std::string& value) {
  _impl_._has_bits_[0] |= 0x00000001u;
  _impl_.name_.Set(value, GetArenaForAllocation());
}
inline std::string* Part::_internal_mutable_name() {
  _impl_._has_bits_[0] |= 0x00000001u;
  return _impl_.name_.Mutable(GetArenaForAllocation());
}
inline std::string* Part::release_name() {
  // @@protoc_insertion_point(field_release:Odb.Lib.Protobuf.ProductModel.Part.name)
  if (!_internal_has_name()) {
    return nullptr;
  }
  _impl_._has_bits_[0] &= ~0x00000001u;
  auto* p = _impl_.name_.Release();
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.name_.IsDefault()) {
    _impl_.name_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  return p;
}
inline void Part::set_allocated_name(std::string* name) {
  if (name != nullptr) {
    _impl_._has_bits_[0] |= 0x00000001u;
  } else {
    _impl_._has_bits_[0] &= ~0x00000001u;
  }
  _impl_.name_.SetAllocated(name, GetArenaForAllocation());
#ifdef PROTOBUF_FORCE_COPY_DEFAULT_STRING
  if (_impl_.name_.IsDefault()) {
    _impl_.name_.Set("", GetArenaForAllocation());
  }
#endif // PROTOBUF_FORCE_COPY_DEFAULT_STRING
  // @@protoc_insertion_point(field_set_allocated:Odb.Lib.Protobuf.ProductModel.Part.name)
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

#include <google/protobuf/port_undef.inc>
#endif  // GOOGLE_PROTOBUF_INCLUDED_GOOGLE_PROTOBUF_INCLUDED_part_2eproto