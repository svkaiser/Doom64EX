#pragma once

#include <prelude.hh>

namespace imp {
  enum class UriSchema {
      file,
      wad
  };

  class Path {
      UriSchema schema_ {};
      String str_ {};

  public:
      Path() = default;
      Path(Path&&) = default;
      Path(const Path&) = default;
      Path& operator=(Path&&) = default;
      Path& operator=(const Path&) = default;

      Path(StringView uri);

      UriSchema schema() const
      { return schema_; }

      const String& str() const
      { return str_;  }

      Path extension();

      Path stem();

      Path filename();
  };
}
