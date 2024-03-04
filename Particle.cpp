#include "Particle.hpp"
#include "vk_initializers.hpp"
#include "Particle.hpp"
#include <random>
#include "vk_pipelines.hpp"
#include "Camera.hpp"

void Particle::initialize(VulkanEngine* engine)
{
  	_engine = engine;
	PARTICLE_COUNT = 64 * 50;
	load_particle();
}

void Particle::guiRender(){}
float rand_lcg(uint *seed) {
    const uint a = 1664525;
    const uint c = 1013904223;
    *seed = a * (*seed) + c;
    return ((float)(*seed) / (float)UINT_MAX);
}
void Particle::load_particle()
{
	std::vector<ParticleObject> ps;
	ps.resize(PARTICLE_COUNT);

	std::default_random_engine rndEngine((unsigned)time(nullptr));
  	std::uniform_real_distribution<float> rndDist(-1.0f, 1.0f);
	std::random_device _rd;
	uint32_t seed = _rd();

	for (int i =0; i < ps.size(); ++i) {
		float sd = (float)(_rd() % 3200) / 320.f; 
		ps[i].position = glm::vec4(0,0,0,sd);
		ps[i].velocity = glm::vec4(0);
	}

	const size_t bufferSize = ps.size() * sizeof(ParticleObject);
	//allocate vertex buffer
	VkBufferCreateInfo stagingBufferInfo = {};
	stagingBufferInfo.pNext = nullptr;
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.size = bufferSize;
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

	//let the VMA library know that this data should be writeable by CPU, but also readable by GPU
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	AllocatedBuffer stagingBuffer;
	//allocate the buffer
	VK_CHECK(vmaCreateBuffer(_engine->_allocator, &stagingBufferInfo, &vmaallocInfo,
		&stagingBuffer.buffer,
		&stagingBuffer.allocation,
		nullptr));

	void* data;
	vmaMapMemory(_engine->_allocator, stagingBuffer.allocation, &data);
	memcpy(data, ps.data(), bufferSize);
	vmaFlushAllocation(_engine->_allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);
	vmaUnmapMemory(_engine->_allocator, stagingBuffer.allocation);

	VkBufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.pNext = nullptr;
	vertexBufferInfo.size = bufferSize;
	vertexBufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vmaallocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	//allocate the buffer

	VK_CHECK(vmaCreateBuffer(_engine->_allocator, &vertexBufferInfo, &vmaallocInfo,
		&_particleBuffer.buffer,
		&_particleBuffer.allocation,
		nullptr));

	_engine->immediate_submit([=](VkCommandBuffer cmd){
		VkBufferCopy copy;
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = bufferSize;
		vkCmdCopyBuffer(cmd, stagingBuffer.buffer, _particleBuffer.buffer, 1, &copy);
	});

	_deletionQueue.push_function([=]() {
	vmaDestroyBuffer(_engine->_allocator, _particleBuffer.buffer, _particleBuffer.allocation);
	});
	vmaDestroyBuffer(_engine->_allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

