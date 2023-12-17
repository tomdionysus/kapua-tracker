//
// Kapua Tracker class
//
// Author: Tom Cully <mail@tomcully.com>
// Copyright (c) Tom Cully 2023 
//
#pragma once

#include <cstdint>
#include <vector>

namespace Kapua {

class Tracker {
 public:
  Tracker();
  ~Tracker();

 protected:
  uint64_t _id;
};

};  // namespace Kapua