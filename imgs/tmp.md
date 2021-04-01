```cpp
if (dindirect && dindirect->buffer)
{
    assert(num_draws == 1);
    if (needs_drawid)
        update_drawid(ctx, draw_id);
    struct zink_resource *indirect = zink_resource(dindirect->buffer);
    zink_batch_reference_resource_rw(batch, indirect, false);
    if (dindirect->indirect_draw_count)
    {
        struct zink_resource *indirect_draw_count = zink_resource(dindirect->indirect_draw_count);
        zink_batch_reference_resource_rw(batch, indirect_draw_count, false);
        screen->vk_CmdDrawIndirectCount(batch->state->cmdbuf, indirect->obj->buffer, dindirect->offset,
                                        indirect_draw_count->obj->buffer, dindirect->indirect_draw_count_offset,
                                        dindirect->draw_count, dindirect->stride);
    }
    else
        vkCmdDrawIndirect(batch->state->cmdbuf, indirect->obj->buffer, dindirect->offset, dindirect->draw_count, dindirect->stride);
}
```

```cpp
struct Base {
    // attributes
}

struct Derived {
    Base base;
    // attributes
}
```

```cpp
void
zink_resource_buffer_barrier(struct zink_context *ctx, struct zink_batch *batch, struct zink_resource *res, VkAccessFlags flags, VkPipelineStageFlags pipeline)
{
   VkMemoryBarrier bmb;
   if (!pipeline)
      pipeline = pipeline_access_stage(flags);
   if (!zink_resource_buffer_needs_barrier(res, flags, pipeline))
      return;

   bmb.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
   bmb.pNext = NULL;
   bmb.srcAccessMask = res->access;
   bmb.dstAccessMask = flags;
   VkCommandBuffer cmdbuf = get_cmdbuf(ctx, res);
   /* only barrier if we're changing layout or doing something besides read -> read */
   vkCmdPipelineBarrier(
      cmdbuf,
      res->access_stage ? res->access_stage : pipeline_access_stage(res->access),
      pipeline,
      0,
      1, &bmb,
      0, NULL,
      0, NULL
   );

   resource_check_defer_barrier(ctx, res, pipeline);

   if (res->unordered_barrier) {
      res->access |= bmb.dstAccessMask;
      res->access_stage |= pipeline;
   } else {
      res->access = bmb.dstAccessMask;
      res->access_stage = pipeline;
   }
}
```