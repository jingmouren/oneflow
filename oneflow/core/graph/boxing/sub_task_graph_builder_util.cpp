#include "oneflow/core/graph/boxing/sub_task_graph_builder_util.h"
#include "oneflow/core/common/balanced_splitter.h"

namespace oneflow {

bool SubTskGphBuilderUtil::IsDeviceTypeCPUOrGPU(const ParallelDesc& parallel_desc) {
  return parallel_desc.device_type() == DeviceType::kCPU
         || parallel_desc.device_type() == DeviceType::kGPU;
}

std::vector<TensorSliceView> SubTskGphBuilderUtil::GetTensorSliceView(
    const int64_t parallel_num, const SbpParallel& sbp_parallel, const BlobDesc& blob_desc) {
  std::vector<Range> ranges(blob_desc.shape().NumAxes());
  FOR_RANGE(int64_t, i, 0, blob_desc.shape().NumAxes()) {
    ranges[i].mut_begin() = 0;
    ranges[i].mut_end() = blob_desc.shape().At(i);
  }
  std::vector<TensorSliceView> views;
  if (sbp_parallel.has_partial_sum_parallel() || sbp_parallel.has_broadcast_parallel()) {
    FOR_RANGE(int64_t, i, 0, parallel_num) { views.emplace_back(ranges); }
  } else if (sbp_parallel.has_split_parallel()) {
    const int64_t axis = sbp_parallel.split_parallel().axis();
    const BalancedSplitter bs(blob_desc.shape().At(axis), parallel_num);
    FOR_RANGE(int64_t, i, 0, parallel_num) {
      if (bs.At(i).size() == 0) {
        views.emplace_back();
      } else {
        ranges[axis] = bs.At(i);
        views.emplace_back(ranges);
      }
    }
  } else {
    UNIMPLEMENTED();
  }
  return views;
}

TensorSliceView SubTskGphBuilderUtil::GetBroadcastTensorSliceView(const BlobDesc& blob_desc) {
  return TensorSliceView(blob_desc.shape());
}

bool SubTskGphBuilderUtil::HasEmptySliceIfSplit(int64_t parallel_num,
                                                const SbpParallel& sbp_parallel,
                                                const BlobDesc& blob_desc) {
  if (sbp_parallel.has_split_parallel()) {
    return blob_desc.shape().At(sbp_parallel.split_parallel().axis()) < parallel_num;
  } else {
    return false;
  }
}

bool SubTskGphBuilderUtil::IsOnSameGPU(const TaskNode* lhs, const TaskNode* rhs) {
  return lhs->machine_id() == rhs->machine_id() && lhs->device_type() == DeviceType::kGPU
         && rhs->device_type() == DeviceType::kGPU && lhs->GpuPhyId() == rhs->GpuPhyId();
}

bool SubTskGphBuilderUtil::IsBoxingS2S(const SbpParallel& src, const SbpParallel& dst) {
  return src.has_split_parallel() && dst.has_split_parallel();
}

bool SubTskGphBuilderUtil::IsBoxingS2B(const SbpParallel& src, const SbpParallel& dst) {
  return src.has_split_parallel() && dst.has_broadcast_parallel();
}

bool SubTskGphBuilderUtil::IsBoxingP2S(const SbpParallel& src, const SbpParallel& dst) {
  return src.has_partial_sum_parallel() && dst.has_split_parallel();
}

bool SubTskGphBuilderUtil::IsBoxingP2B(const SbpParallel& src, const SbpParallel& dst) {
  return src.has_partial_sum_parallel() && dst.has_broadcast_parallel();
}

bool SubTskGphBuilderUtil::IsBoxingB2B(const SbpParallel& src, const SbpParallel& dst) {
  return src.has_broadcast_parallel() && dst.has_broadcast_parallel();
}

bool SubTskGphBuilderUtil::BlobHasDynamicShape(const BlobDesc& blob_desc) {
  return blob_desc.has_dim0_valid_num_field() || blob_desc.has_dim1_valid_num_field()
         || blob_desc.has_dim2_valid_num_field();
}

bool SubTskGphBuilderUtil::IsErrorBoxingNotSupported(const ErrorProto& error) {
  return error.has_boxing_error() && error.boxing_error() == BoxingError::kNotSupported;
}

}  // namespace oneflow