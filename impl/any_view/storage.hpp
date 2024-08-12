#ifndef LIBCPP__RANGE_STORAGE_HPP
#define LIBCPP__RANGE_STORAGE_HPP

#include <concepts>
#include <memory>

namespace std::ranges::detail {

template <class T>
struct type {
  using t = T;
};

template <size_t Size, size_t Align, bool Copyable>
struct storage {
  constexpr storage() = default;

  template <class T, class... Args>
    requires constructible_from<T, Args &&...>
  constexpr storage(type<T>, Args &&...args) : vtable_(&vtable_instance<T>) {
    if constexpr (use_small_buffer<T>) {
      std::construct_at(get_ptr<T>(), std::forward<Args>(args)...);
    } else {
      heap_ptr_ = new T(std::forward<Args>(args)...);
    }
  }

  constexpr storage(const storage &other)
    requires Copyable
  {
    if (!other.is_singular()) {
      (*(other.vtable_->copy_))(other, *this);
    }
  }

  constexpr storage(storage &&other) noexcept {
    if (!other.is_singular()) {
      (*(other.vtable_->move_))(std::move(other), *this);
    }
  }

  constexpr storage &operator=(const storage &other)
    requires Copyable
  {
    storage(other).swap(*this);
    return *this;
  }

  constexpr storage &operator=(storage &&other) noexcept {
    storage(std::move(other)).swap(*this);
    return *this;
  }

  constexpr ~storage() {
    if (!is_singular()) {
      (*(vtable_->destroy_))(*this);
    }
  }

  template <class T, class Self>
  constexpr decltype(auto) get_ptr(this Self &&self) {
    constexpr bool is_this_const =
        std::is_const_v<std::remove_reference_t<Self>>;
    using ptr = std::conditional_t<is_this_const, const T *, T *>;
    if constexpr (use_small_buffer<T>) {
      return reinterpret_cast<ptr>(&self.buf_.buf_[0]);
    } else {
      return static_cast<ptr>(self.heap_ptr_);
    }
  }

  constexpr void swap(storage &other) noexcept {
    if (this == &other) return;

    if (!is_singular() && !other.is_singular()) {
      storage tmp(singular_tag{});
      (*other.vtable_->destructive_move_)(std::move(other), tmp);
      (*vtable_->destructive_move_)(std::move(*this), other);
      (*tmp.vtable_->destructive_move_)(std::move(tmp), *this);
      tmp.vtable_ = nullptr;
    } else if (!is_singular()) {
      (*vtable_->destructive_move_)(std::move(*this), other);
      vtable_ = nullptr;
    } else if (!other.is_singular()) {
      (*other.vtable_->destructive_move_)(std::move(other), *this);
      other.vtable_ = nullptr;
    }
  }

  constexpr bool is_singular() const { return !vtable_; }

  template <class T>
  static constexpr bool unittest_is_small() {
    return use_small_buffer<T>;
  }

 private:
  struct singular_tag {};
  explicit constexpr storage(singular_tag) {};

  struct Buffer {
    static constexpr size_t size = Size;
    alignas(Align) char buf_[size];
  };

  template <class Tp>
  static constexpr bool use_small_buffer =
      sizeof(Tp) <= sizeof(Buffer) && alignof(Buffer) % alignof(Tp) == 0 &&
      is_nothrow_move_constructible_v<Tp>;

  union {
    void *heap_ptr_;
    Buffer buf_;
  };

  struct empty {};
  struct copyable_vtable {
    void (*copy_)(const storage &, storage &);
  };
  struct vtable : conditional_t<Copyable, copyable_vtable, empty> {
    void (*destroy_)(storage &);
    void (*move_)(storage &&, storage &);
    void (*destructive_move_)(storage &&, storage &);
  };

  vtable const *vtable_ = nullptr;

  template <class Tp>
  consteval static vtable gen_vtable() {
    vtable vt{};
    if constexpr (use_small_buffer<Tp>) {
      vt.destroy_ = [](storage &self) noexcept {
        std::destroy_at(self.get_ptr<Tp>());
      };
      vt.move_ = [](storage &&self, storage &dest) noexcept {
        std::construct_at(dest.get_ptr<Tp>(), std::move((*self.get_ptr<Tp>())));
        dest.vtable_ = self.vtable_;
      };
      vt.destructive_move_ = [](storage &&self, storage &dest) noexcept {
        std::construct_at(dest.get_ptr<Tp>(), std::move((*self.get_ptr<Tp>())));
        dest.vtable_ = self.vtable_;
        std::destroy_at(self.get_ptr<Tp>());
      };
      if constexpr (Copyable) {
        // may throw, but self is unchanged after throw
        vt.copy_ = [](storage const &self, storage &dest) {
          std::construct_at(dest.get_ptr<Tp>(), *self.get_ptr<Tp>());
          dest.vtable_ = self.vtable_;
        };
      }
    } else {
      vt.destroy_ = [](storage &self) noexcept {
        delete static_cast<Tp *>(self.heap_ptr_);
      };
      vt.move_ = [](storage &&self, storage &dest) noexcept {
        dest.heap_ptr_ = self.heap_ptr_;
        dest.vtable_ = self.vtable_;
        self.heap_ptr_ = nullptr;
        self.vtable_ = nullptr;
      };
      vt.destructive_move_ = vt.move_;
      if constexpr (Copyable) {
        vt.copy_ = [](storage const &self, storage &dest) {
          // may throw, but self is unchanged after throw
          dest.heap_ptr_ = new Tp(*static_cast<const Tp *>(self.heap_ptr_));
          dest.vtable_ = self.vtable_;
        };
      }
    }
    return vt;
  }

  template <class Tp>
  static constexpr vtable vtable_instance = gen_vtable<Tp>();
};

}  // namespace std::ranges::detail

#endif
