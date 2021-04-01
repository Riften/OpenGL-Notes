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
        screen->vk_CmdDrawIndexedIndirectCount(batch->state->cmdbuf, indirect->obj->buffer, dindirect->offset,
                                               indirect_draw_count->obj->buffer, dindirect->indirect_draw_count_offset,
                                               dindirect->draw_count, dindirect->stride);
    }
    else
        vkCmdDrawIndexedIndirect(batch->state->cmdbuf, indirect->obj->buffer, dindirect->offset, dindirect->draw_count, dindirect->stride);
}