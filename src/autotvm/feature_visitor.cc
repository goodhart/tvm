/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*!
 *  Copyright (c) 2018 by Contributors
 * \file feature_visitor.cc
 * \brief Base class for feature extractor.
 *        These features are used for machine learning cost model
 */

#include "feature_visitor.h"

namespace tvm {
namespace autotvm {

// for loop
void FeatureVisitor::Visit_(const For *op) {
  const auto *extent = op->extent.as<IntImm>();
  int64_t loop_extent = -1;
  if (extent != nullptr)
    loop_extent = extent->value;
  AnnotationType ann = kSerial;
  switch (op->for_type) {
    case ForType ::Parallel:
      ann = kParallel;
      break;
    case ForType::Unrolled:
      ann = kUnrolled;
      break;
    case ForType::Vectorized:
      ann = kVectorized;
      break;
    case ForType::Serial:
      ann = kSerial;
      break;
  }

  if (EnterItervar_(op->loop_var, loop_extent, ann)) {
    IRVisitor::Visit_(op);
    ExitItervar_();
  }
}

// parallel axis, virtual thread
void FeatureVisitor::Visit_(const AttrStmt *op) {
  if (op->attr_key == attr::thread_extent ||
      op->attr_key == attr::virtual_thread) {
    VarExpr var = op->node.as<tvm::IterVarNode>()->var;
    const auto *extent = op->value.as<IntImm>();
    CHECK(extent);

    std::string name = var.get()->name_hint;
    AnnotationType ann = kParallel;
    if (op->attr_key == attr::thread_extent) {
      if (name == "blockIdx.x")
        ann = kBlockX;
      else if (name == "blockIdx.y")
        ann = kBlockY;
      else if (name == "blockIdx.z")
        ann = kBlockZ;
      else if (name == "threadIdx.x")
        ann = kThreadX;
      else if (name == "threadIdx.y")
        ann = kThreadY;
      else if (name == "threadIdx.z")
        ann = kThreadZ;
      else
        LOG(FATAL) << "invalid thread itervar " + name;
    } else {
      ann = kVirtualThread;
    }

    if (EnterItervar_(var, extent->value, ann)) {
      IRVisitor::Visit_(op);
      ExitItervar_();
    }
  } else {
    IRVisitor::Visit_(op);
  }
}

// memory access
void FeatureVisitor::Visit_(const Load *op) {
  EnterMem_(op->buffer_var, op->index);
  IRVisitor::Visit_(op);
  ExitMem_();
}

void FeatureVisitor::Visit_(const Store *op) {
  EnterMem_(op->buffer_var, op->index);
  IRVisitor::Visit_(op);
  ExitMem_();
}

}  // namespace autotvm
}  // namespace tvm
