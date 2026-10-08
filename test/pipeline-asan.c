#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "../framegen.c"

static int descriptor_calls, pipeline_calls;
static uintptr_t next_handle = 0x1000;
static uintptr_t bump(void) { next_handle += 8; return next_handle; }

static VkResult VKAPI_CALL m_sampler(VkDevice d, const VkSamplerCreateInfo *ci,
                                     const VkAllocationCallbacks *a, VkSampler *out) {
    (void)d; (void)ci; (void)a;
    *out = (VkSampler)bump();
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL m_dsl(VkDevice d, const VkDescriptorSetLayoutCreateInfo *ci,
                                 const VkAllocationCallbacks *a, VkDescriptorSetLayout *out) {
    (void)d; (void)a;
    const struct pipe_spec *s = &pipe_specs[descriptor_calls];
    assert(ci->bindingCount == s->n);
    for (uint32_t i = 0; i < ci->bindingCount; i++) {
        assert(ci->pBindings[i].binding == i);
        assert(ci->pBindings[i].descriptorCount == 1);
        assert(ci->pBindings[i].stageFlags == VK_SHADER_STAGE_COMPUTE_BIT);
        assert(ci->pBindings[i].descriptorType == s->types[i]);
    }
    descriptor_calls++;
    *out = (VkDescriptorSetLayout)bump();
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL m_layout(VkDevice d, const VkPipelineLayoutCreateInfo *ci,
                                    const VkAllocationCallbacks *a, VkPipelineLayout *out) {
    (void)d; (void)ci; (void)a;
    *out = (VkPipelineLayout)bump();
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL m_shader(VkDevice d, const VkShaderModuleCreateInfo *ci,
                                    const VkAllocationCallbacks *a, VkShaderModule *out) {
    (void)d; (void)ci; (void)a;
    *out = (VkShaderModule)bump();
    return VK_SUCCESS;
}

static VkResult VKAPI_CALL m_pipeline(VkDevice d, VkPipelineCache c, uint32_t n,
                                      const VkComputePipelineCreateInfo *ci,
                                      const VkAllocationCallbacks *a, VkPipeline *out) {
    (void)d; (void)c; (void)n; (void)ci; (void)a;
    pipeline_calls++;
    *out = (VkPipeline)bump();
    return VK_SUCCESS;
}

static void VKAPI_CALL m_destroy_shader(VkDevice d, VkShaderModule m,
                                        const VkAllocationCallbacks *a) {
    (void)d; (void)m; (void)a;
}

int main(void) {
    struct dev d = {0};
    d.CreateSampler = m_sampler;
    d.CreateDescriptorSetLayout = m_dsl;
    d.CreatePipelineLayout = m_layout;
    d.CreateShaderModule = m_shader;
    d.CreateComputePipelines = m_pipeline;
    d.DestroyShaderModule = m_destroy_shader;

    assert(pipelines_create(&d));
    assert(descriptor_calls == NPIPE);
    assert(pipeline_calls == NPIPE);

    assert(pipelines_create(&d));
    assert(descriptor_calls == NPIPE);
    assert(pipeline_calls == NPIPE);

    printf("pipeline-asan OK: dsl=%d pipe=%d\n", descriptor_calls, pipeline_calls);
    return 0;
}
