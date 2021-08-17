/*
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: MIT
 */

#include "vkr_descriptor_set.h"

#include "vkr_descriptor_set_gen.h"

static void
vkr_dispatch_vkGetDescriptorSetLayoutSupport(
   UNUSED struct vn_dispatch_context *dispatch,
   struct vn_command_vkGetDescriptorSetLayoutSupport *args)
{
   vn_replace_vkGetDescriptorSetLayoutSupport_args_handle(args);
   vkGetDescriptorSetLayoutSupport(args->device, args->pCreateInfo, args->pSupport);
}

static void
vkr_dispatch_vkCreateDescriptorSetLayout(
   struct vn_dispatch_context *dispatch,
   struct vn_command_vkCreateDescriptorSetLayout *args)
{
   vkr_descriptor_set_layout_create_and_add(dispatch->data, args);
}

static void
vkr_dispatch_vkDestroyDescriptorSetLayout(
   struct vn_dispatch_context *dispatch,
   struct vn_command_vkDestroyDescriptorSetLayout *args)
{
   vkr_descriptor_set_layout_destroy_and_remove(dispatch->data, args);
}

static void
vkr_dispatch_vkCreateDescriptorPool(struct vn_dispatch_context *dispatch,
                                    struct vn_command_vkCreateDescriptorPool *args)
{
   struct vkr_descriptor_pool *pool =
      vkr_descriptor_pool_create_and_add(dispatch->data, args);
   if (!pool)
      return;

   list_inithead(&pool->descriptor_sets);
}

static void
vkr_dispatch_vkDestroyDescriptorPool(struct vn_dispatch_context *dispatch,
                                     struct vn_command_vkDestroyDescriptorPool *args)
{
   struct vkr_context *ctx = dispatch->data;
   struct vkr_descriptor_pool *pool =
      vkr_descriptor_pool_from_handle(args->descriptorPool);

   if (!pool)
      return;

   vkr_context_remove_objects(ctx, &pool->descriptor_sets);

   vkr_descriptor_pool_destroy_and_remove(ctx, args);
}

static void
vkr_dispatch_vkResetDescriptorPool(struct vn_dispatch_context *dispatch,
                                   struct vn_command_vkResetDescriptorPool *args)
{
   struct vkr_context *ctx = dispatch->data;

   struct vkr_descriptor_pool *pool =
      vkr_descriptor_pool_from_handle(args->descriptorPool);
   if (!pool) {
      vkr_cs_decoder_set_fatal(&ctx->decoder);
      return;
   }

   vn_replace_vkResetDescriptorPool_args_handle(args);
   args->ret = vkResetDescriptorPool(args->device, args->descriptorPool, args->flags);

   vkr_context_remove_objects(ctx, &pool->descriptor_sets);
   list_inithead(&pool->descriptor_sets);
}

static void
vkr_dispatch_vkAllocateDescriptorSets(struct vn_dispatch_context *dispatch,
                                      struct vn_command_vkAllocateDescriptorSets *args)
{
   struct vkr_context *ctx = dispatch->data;
   struct vkr_device *dev = vkr_device_from_handle(args->device);
   struct vkr_descriptor_pool *pool =
      vkr_descriptor_pool_from_handle(args->pAllocateInfo->descriptorPool);
   struct object_array arr;

   if (!pool) {
      vkr_cs_decoder_set_fatal(&ctx->decoder);
      return;
   }

   if (vkr_descriptor_set_create_array(ctx, args, &arr) != VK_SUCCESS)
      return;

   vkr_descriptor_set_add_array(ctx, dev, pool, &arr);
}

static void
vkr_dispatch_vkFreeDescriptorSets(struct vn_dispatch_context *dispatch,
                                  struct vn_command_vkFreeDescriptorSets *args)
{
   struct vkr_context *ctx = dispatch->data;
   struct list_head free_list;

   /* args->pDescriptorSets is marked noautovalidity="true" */
   if (args->descriptorSetCount && !args->pDescriptorSets) {
      vkr_cs_decoder_set_fatal(&ctx->decoder);
      return;
   }

   vkr_descriptor_set_destroy_driver_handles(ctx, args, &free_list);
   vkr_context_remove_objects(ctx, &free_list);

   args->ret = VK_SUCCESS;
}

static void
vkr_dispatch_vkUpdateDescriptorSets(UNUSED struct vn_dispatch_context *dispatch,
                                    struct vn_command_vkUpdateDescriptorSets *args)
{
   vn_replace_vkUpdateDescriptorSets_args_handle(args);
   vkUpdateDescriptorSets(args->device, args->descriptorWriteCount,
                          args->pDescriptorWrites, args->descriptorCopyCount,
                          args->pDescriptorCopies);
}

static void
vkr_dispatch_vkCreateDescriptorUpdateTemplate(
   struct vn_dispatch_context *dispatch,
   struct vn_command_vkCreateDescriptorUpdateTemplate *args)
{
   vkr_descriptor_update_template_create_and_add(dispatch->data, args);
}

static void
vkr_dispatch_vkDestroyDescriptorUpdateTemplate(
   struct vn_dispatch_context *dispatch,
   struct vn_command_vkDestroyDescriptorUpdateTemplate *args)
{
   vkr_descriptor_update_template_destroy_and_remove(dispatch->data, args);
}

void
vkr_context_init_descriptor_set_layout_dispatch(struct vkr_context *ctx)
{
   struct vn_dispatch_context *dispatch = &ctx->dispatch;

   dispatch->dispatch_vkGetDescriptorSetLayoutSupport =
      vkr_dispatch_vkGetDescriptorSetLayoutSupport;
   dispatch->dispatch_vkCreateDescriptorSetLayout =
      vkr_dispatch_vkCreateDescriptorSetLayout;
   dispatch->dispatch_vkDestroyDescriptorSetLayout =
      vkr_dispatch_vkDestroyDescriptorSetLayout;
}

void
vkr_context_init_descriptor_pool_dispatch(struct vkr_context *ctx)
{
   struct vn_dispatch_context *dispatch = &ctx->dispatch;

   dispatch->dispatch_vkCreateDescriptorPool = vkr_dispatch_vkCreateDescriptorPool;
   dispatch->dispatch_vkDestroyDescriptorPool = vkr_dispatch_vkDestroyDescriptorPool;
   dispatch->dispatch_vkResetDescriptorPool = vkr_dispatch_vkResetDescriptorPool;
}

void
vkr_context_init_descriptor_set_dispatch(struct vkr_context *ctx)
{
   struct vn_dispatch_context *dispatch = &ctx->dispatch;

   dispatch->dispatch_vkAllocateDescriptorSets = vkr_dispatch_vkAllocateDescriptorSets;
   dispatch->dispatch_vkFreeDescriptorSets = vkr_dispatch_vkFreeDescriptorSets;
   dispatch->dispatch_vkUpdateDescriptorSets = vkr_dispatch_vkUpdateDescriptorSets;
}

void
vkr_context_init_descriptor_update_template_dispatch(struct vkr_context *ctx)
{
   struct vn_dispatch_context *dispatch = &ctx->dispatch;

   dispatch->dispatch_vkCreateDescriptorUpdateTemplate =
      vkr_dispatch_vkCreateDescriptorUpdateTemplate;
   dispatch->dispatch_vkDestroyDescriptorUpdateTemplate =
      vkr_dispatch_vkDestroyDescriptorUpdateTemplate;
   dispatch->dispatch_vkUpdateDescriptorSetWithTemplate = NULL;
}
