#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <vector>
#include <stdexcept>
#include <vector>

const uint32_t WIDTH = 1280;
const uint32_t HEIGHT = 720;
const int WORLD_SIZE = 64;
const int WORLD_HEIGHT = 32;

enum BlockType : uint8_t {
    BLOCK_AIR = 0,
    BLOCK_GRASS,
    BLOCK_DIRT,
    BLOCK_STONE,
    BLOCK_SAND,
    BLOCK_WOOD,
    BLOCK_WATER,
    BLOCK_BRICK,
    BLOCK_COUNT
};

struct BlockColor {
    float r, g, b;
};

BlockColor getBlockColor(BlockType type) {
    switch (type) {
        case BLOCK_GRASS:  return {0.30f, 0.70f, 0.20f};
        case BLOCK_DIRT:   return {0.55f, 0.35f, 0.18f};
        case BLOCK_STONE:  return {0.50f, 0.50f, 0.50f};
        case BLOCK_SAND:   return {0.85f, 0.80f, 0.55f};
        case BLOCK_WOOD:   return {0.55f, 0.35f, 0.15f};
        case BLOCK_WATER:  return {0.20f, 0.40f, 0.85f};
        case BLOCK_BRICK:  return {0.70f, 0.25f, 0.20f};
        default:           return {0.80f, 0.20f, 0.80f};
    }
}

const char* getBlockName(BlockType type) {
    switch (type) {
        case BLOCK_GRASS:  return "Grass";
        case BLOCK_DIRT:   return "Dirt";
        case BLOCK_STONE:  return "Stone";
        case BLOCK_SAND:   return "Sand";
        case BLOCK_WOOD:   return "Wood";
        case BLOCK_WATER:  return "Water";
        case BLOCK_BRICK:  return "Brick";
        default:           return "Unknown";
    }
}

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normal;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attrs{};
        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(Vertex, pos);
        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset = offsetof(Vertex, color);
        attrs[2].binding = 0;
        attrs[2].location = 2;
        attrs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[2].offset = offsetof(Vertex, normal);
        return attrs;
    }
};

struct UBO {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 lightDir;
    alignas(16) float time;
};

struct Vertex2D {
    glm::vec2 pos;
    glm::vec3 color;
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription b{};
        b.binding = 0; b.stride = sizeof(Vertex2D); b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return b;
    }
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> a{};
        a[0].binding = 0; a[0].location = 0; a[0].format = VK_FORMAT_R32G32_SFLOAT; a[0].offset = offsetof(Vertex2D, pos);
        a[1].binding = 0; a[1].location = 1; a[1].format = VK_FORMAT_R32G32B32_SFLOAT; a[1].offset = offsetof(Vertex2D, color);
        return a;
    }
};

struct UBO2D { alignas(16) glm::mat4 proj; };

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Camera {
    glm::vec3 position = {WORLD_SIZE / 2.0f, 7.0f, WORLD_SIZE / 2.0f + 15.0f};
    float yaw = -90.0f;
    float pitch = 0.0f;
    float speed = 6.0f;
    float flySpeed = 14.0f;
    float sensitivity = 0.1f;
    float fov = 70.0f;
    static constexpr float eyeHeight = 1.6f;

    bool flying = false;
    float velocityY = 0.0f;
    bool onGround = false;
    bool spaceWasPressed = false;
    static constexpr float gravity = -28.0f;
    static constexpr float jumpVelocity = 9.0f;

    glm::vec3 front() const {
        return glm::normalize(glm::vec3(
            cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
            sin(glm::radians(pitch)),
            sin(glm::radians(yaw)) * cos(glm::radians(pitch))
        ));
    }

    glm::vec3 right() const { return glm::normalize(glm::cross(front(), glm::vec3(0, 1, 0))); }

    glm::vec3 flatFront() const {
        return glm::normalize(glm::vec3(cos(glm::radians(yaw)), 0.0f, sin(glm::radians(yaw))));
    }

    void processMovement(int direction, float dt) {
        float v = (flying ? flySpeed : speed) * dt;
        glm::vec3 fwd = flying ? front() : flatFront();
        if (direction == 0) position += fwd * v;
        if (direction == 1) position -= fwd * v;
        if (direction == 2) position -= right() * v;
        if (direction == 3) position += right() * v;
        if (flying) {
            if (direction == 4) position.y += flySpeed * dt;
            if (direction == 5) position.y -= flySpeed * dt;
        }
    }

    void processMouse(float xoffset, float yoffset) {
        yaw += xoffset * sensitivity;
        pitch += yoffset * sensitivity;
        pitch = std::clamp(pitch, -89.0f, 89.0f);
    }

    glm::mat4 viewMatrix() const {
        return glm::lookAt(position, position + front(), glm::vec3(0, 1, 0));
    }
};

struct World {
    uint8_t blocks[WORLD_SIZE][WORLD_HEIGHT][WORLD_SIZE];

    void generate() {
        for (int x = 0; x < WORLD_SIZE; x++) {
            for (int z = 0; z < WORLD_SIZE; z++) {
                blocks[x][0][z] = BLOCK_STONE;
                for (int y = 1; y < 4; y++) blocks[x][y][z] = BLOCK_DIRT;
                blocks[x][4][z] = BLOCK_GRASS;
                for (int y = 5; y < WORLD_HEIGHT; y++) blocks[x][y][z] = BLOCK_AIR;
            }
        }
    }

    BlockType get(int x, int y, int z) const {
        if (x < 0 || x >= WORLD_SIZE || y < 0 || y >= WORLD_HEIGHT || z < 0 || z >= WORLD_SIZE)
            return BLOCK_AIR;
        return static_cast<BlockType>(blocks[x][y][z]);
    }

    void set(int x, int y, int z, BlockType t) {
        if (x >= 0 && x < WORLD_SIZE && y >= 0 && y < WORLD_HEIGHT && z >= 0 && z < WORLD_SIZE)
            blocks[x][y][z] = t;
    }
};

inline void applyPhysics(Camera& camera, const World& world, float dt) {
    if (camera.flying) { camera.onGround = false; return; }

    camera.velocityY += Camera::gravity * dt;
    if (camera.velocityY < -50.0f) camera.velocityY = -50.0f;
    camera.position.y += camera.velocityY * dt;

    int bx = static_cast<int>(floor(camera.position.x));
    int bz = static_cast<int>(floor(camera.position.z));
    float groundY = -1.0f;
    for (int y = WORLD_HEIGHT - 1; y >= 0; y--) {
        if (world.get(bx, y, bz) != BLOCK_AIR) {
            groundY = static_cast<float>(y + 1);
            break;
        }
    }
    float feetY = camera.position.y - Camera::eyeHeight;
    if (feetY <= groundY) {
        camera.position.y = groundY + Camera::eyeHeight;
        camera.velocityY = 0;
        camera.onGround = true;
    } else {
        camera.onGround = false;
    }
}

class VoxelEngine {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window = nullptr;
    Camera camera;
    World world;
    BlockType selectedBlock = BLOCK_GRASS;
    bool firstMouse = true;
    double lastMouseX = WIDTH / 2.0, lastMouseY = HEIGHT / 2.0;
    std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    // Vulkan
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    VkFormat depthFormat;
    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    // Mesh
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexMemory;
    uint32_t indexCount = 0;
    bool meshDirty = true;

    // Uniform
    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    uint32_t currentFrame = 0;
    bool framebufferResized = false;

    // Highlight
    VkBuffer highlightVB, highlightIB;
    VkDeviceMemory highlightVM, highlightIM;
    uint32_t highlightIndexCount = 0;
    VkPipeline linePipeline;
    glm::ivec3 highlightedBlock{-1, -1, -1};

    // 2D HUD
    VkDescriptorSetLayout hudDescriptorSetLayout;
    VkPipelineLayout hudPipelineLayout;
    VkPipeline hudPipeline;
    std::vector<VkBuffer> hudUniformBuffers;
    std::vector<VkDeviceMemory> hudUniformBuffersMemory;
    std::vector<void*> hudUniformBuffersMapped;
    VkDescriptorPool hudDescriptorPool;
    std::vector<VkDescriptorSet> hudDescriptorSets;
    VkBuffer hudVB = VK_NULL_HANDLE;
    VkDeviceMemory hudVM = VK_NULL_HANDLE;
    uint32_t hudVertexCount = 0;

    static const int MAX_FRAMES_IN_FLIGHT = 2;

    // ==================== INIT ====================

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Voxel", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    static void framebufferResizeCallback(GLFWwindow* w, int, int) {
        auto app = reinterpret_cast<VoxelEngine*>(glfwGetWindowUserPointer(w));
        app->framebufferResized = true;
    }

    static void mouseCallback(GLFWwindow* w, double xpos, double ypos) {
        auto app = reinterpret_cast<VoxelEngine*>(glfwGetWindowUserPointer(w));
        if (app->firstMouse) {
            app->lastMouseX = xpos;
            app->lastMouseY = ypos;
            app->firstMouse = false;
        }
        float xoff = static_cast<float>(xpos - app->lastMouseX);
        float yoff = static_cast<float>(app->lastMouseY - ypos);
        app->lastMouseX = xpos;
        app->lastMouseY = ypos;
        app->camera.processMouse(xoff, yoff);
    }

    static void mouseButtonCallback(GLFWwindow* w, int button, int action, int) {
        if (action != GLFW_PRESS) return;
        auto app = reinterpret_cast<VoxelEngine*>(glfwGetWindowUserPointer(w));
        if (button == GLFW_MOUSE_BUTTON_LEFT) app->breakBlock();
        if (button == GLFW_MOUSE_BUTTON_RIGHT) app->placeBlock();
    }

    static void keyCallback(GLFWwindow* w, int key, int, int action, int) {
        if (action != GLFW_PRESS) return;
        auto app = reinterpret_cast<VoxelEngine*>(glfwGetWindowUserPointer(w));
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w, true);
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_7) {
            app->selectedBlock = static_cast<BlockType>(key - GLFW_KEY_1 + 1);
            std::cout << "Selected: " << getBlockName(app->selectedBlock) << "\n";
        }
    }

    // ==================== VULKAN ====================

    void initVulkan() {
        world.generate();
        createInstance();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        depthFormat = findSupportedDepthFormat();
        createRenderPass();
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createVertexBuffer();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createCommandBuffers();
        createSyncObjects();
        createHighlightBuffers();
        createHudPipeline();
        createHudUniformBuffers();
        createHudDescriptorPool();
        createHudDescriptorSets();
    }

    void createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan Voxel";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "VoxelEngine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        uint32_t glfwExtCount = 0;
        const char** glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = glfwExtCount;
        createInfo.ppEnabledExtensionNames = glfwExts;
        createInfo.enabledLayerCount = 0;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
            throw std::runtime_error("Failed to create instance!");
    }

    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
            throw std::runtime_error("Failed to create surface!");
    }

    void pickPhysicalDevice() {
        uint32_t count = 0;
        vkEnumeratePhysicalDevices(instance, &count, nullptr);
        if (count == 0) throw std::runtime_error("No GPU with Vulkan!");
        std::vector<VkPhysicalDevice> devices(count);
        vkEnumeratePhysicalDevices(instance, &count, devices.data());
        for (auto& d : devices) {
            if (isDeviceSuitable(d)) { physicalDevice = d; break; }
        }
        if (physicalDevice == VK_NULL_HANDLE) throw std::runtime_error("No suitable GPU!");
    }

    bool isDeviceSuitable(VkPhysicalDevice d) {
        auto indices = findQueueFamilies(d);
        bool extSupported = checkDeviceExtensionSupport(d);
        bool swapchainOk = false;
        if (extSupported) {
            auto details = querySwapChainSupport(d);
            swapchainOk = !details.formats.empty() && !details.presentModes.empty();
        }
        return indices.isComplete() && extSupported && swapchainOk;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice d) {
        uint32_t count;
        vkEnumerateDeviceExtensionProperties(d, nullptr, &count, nullptr);
        std::vector<VkExtensionProperties> available(count);
        vkEnumerateDeviceExtensionProperties(d, nullptr, &count, available.data());
        std::set<std::string> required = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        for (auto& e : available) required.erase(e.extensionName);
        return required.empty();
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice d) {
        QueueFamilyIndices indices;
        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(d, &count, nullptr);
        std::vector<VkQueueFamilyProperties> families(count);
        vkGetPhysicalDeviceQueueFamilyProperties(d, &count, families.data());
        for (uint32_t i = 0; i < count; i++) {
            if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) indices.graphicsFamily = i;
            VkBool32 present = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(d, i, surface, &present);
            if (present) indices.presentFamily = i;
            if (indices.isComplete()) break;
        }
        return indices;
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice d) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(d, surface, &details.capabilities);
        uint32_t count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(d, surface, &count, nullptr);
        details.formats.resize(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(d, surface, &count, details.formats.data());
        vkGetPhysicalDeviceSurfacePresentModesKHR(d, surface, &count, nullptr);
        details.presentModes.resize(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(d, surface, &count, details.presentModes.data());
        return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& f) {
        for (auto& s : f)
            if (s.format == VK_FORMAT_B8G8R8A8_SRGB && s.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                return s;
        return f[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& m) {
        for (auto& p : m)
            if (p == VK_PRESENT_MODE_MAILBOX_KHR) return p;
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& c) {
        if (c.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return c.currentExtent;
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        VkExtent2D e = {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
        e.width = std::clamp(e.width, c.minImageExtent.width, c.maxImageExtent.width);
        e.height = std::clamp(e.height, c.minImageExtent.height, c.maxImageExtent.height);
        return e;
    }

    void createLogicalDevice() {
        auto indices = findQueueFamilies(physicalDevice);
        std::vector<VkDeviceQueueCreateInfo> queueInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
        float queuePriority = 1.0f;
        for (uint32_t qf : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo qi{};
            qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qi.queueFamilyIndex = qf;
            qi.queueCount = 1;
            qi.pQueuePriorities = &queuePriority;
            queueInfos.push_back(qi);
        }
        VkPhysicalDeviceFeatures deviceFeatures{};
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
        createInfo.pQueueCreateInfos = queueInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 1;
        const char* ext = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        createInfo.ppEnabledExtensionNames = &ext;
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
            throw std::runtime_error("Failed to create logical device!");
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapChain() {
        auto details = querySwapChainSupport(physicalDevice);
        auto format = chooseSwapSurfaceFormat(details.formats);
        auto mode = chooseSwapPresentMode(details.presentModes);
        auto extent = chooseSwapExtent(details.capabilities);
        uint32_t imageCount = details.capabilities.minImageCount + 1;
        if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
            imageCount = details.capabilities.maxImageCount;
        VkSwapchainCreateInfoKHR ci{};
        ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        ci.surface = surface;
        ci.minImageCount = imageCount;
        ci.imageFormat = format.format;
        ci.imageColorSpace = format.colorSpace;
        ci.imageExtent = extent;
        ci.imageArrayLayers = 1;
        ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        auto indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
        if (indices.graphicsFamily != indices.presentFamily) {
            ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            ci.queueFamilyIndexCount = 2;
            ci.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        ci.preTransform = details.capabilities.currentTransform;
        ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        ci.presentMode = mode;
        ci.clipped = VK_TRUE;
        ci.oldSwapchain = VK_NULL_HANDLE;
        if (vkCreateSwapchainKHR(device, &ci, nullptr, &swapChain) != VK_SUCCESS)
            throw std::runtime_error("Failed to create swapchain!");
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());
        swapChainImageFormat = format.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());
        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo ci{};
            ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            ci.image = swapChainImages[i];
            ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
            ci.format = swapChainImageFormat;
            ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ci.subresourceRange.baseMipLevel = 0;
            ci.subresourceRange.levelCount = 1;
            ci.subresourceRange.baseArrayLayer = 0;
            ci.subresourceRange.layerCount = 1;
            if (vkCreateImageView(device, &ci, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create image view!");
        }
    }

    VkFormat findSupportedDepthFormat() {
        std::vector<VkFormat> candidates = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM};
        for (auto fmt : candidates) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, fmt, &props);
            if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
                return fmt;
        }
        throw std::runtime_error("No supported depth format!");
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        VkAttachmentReference colorRef{};
        colorRef.attachment = 0;
        colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        VkAttachmentReference depthRef{};
        depthRef.attachment = 1;
        depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorRef;
        subpass.pDepthStencilAttachment = &depthRef;

        VkSubpassDependency dep{};
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;
        dep.dstSubpass = 0;
        dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.srcAccessMask = 0;
        dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
        VkRenderPassCreateInfo rpInfo{};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        rpInfo.pAttachments = attachments.data();
        rpInfo.subpassCount = 1;
        rpInfo.pSubpasses = &subpass;
        rpInfo.dependencyCount = 1;
        rpInfo.pDependencies = &dep;
        if (vkCreateRenderPass(device, &rpInfo, nullptr, &renderPass) != VK_SUCCESS)
            throw std::runtime_error("Failed to create render pass!");
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 0;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        VkDescriptorSetLayoutCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ci.bindingCount = 1;
        ci.pBindings = &uboBinding;
        if (vkCreateDescriptorSetLayout(device, &ci, nullptr, &descriptorSetLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout!");
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ci.codeSize = code.size();
        ci.pCode = reinterpret_cast<const uint32_t*>(code.data());
        VkShaderModule m;
        if (vkCreateShaderModule(device, &ci, nullptr, &m) != VK_SUCCESS)
            throw std::runtime_error("Failed to create shader module!");
        return m;
    }

    std::vector<char> readFile(const std::string& filename) {
        std::ifstream f(filename, std::ios::ate | std::ios::binary);
        if (!f.is_open()) throw std::runtime_error("Failed to open: " + filename);
        size_t size = static_cast<size_t>(f.tellg());
        std::vector<char> buf(size);
        f.seekg(0);
        f.read(buf.data(), size);
        return buf;
    }

    void createGraphicsPipeline() {
        auto vertCode = readFile("shaders/shader.vert.spv");
        auto fragCode = readFile("shaders/shader.frag.spv");
        VkShaderModule vertModule = createShaderModule(vertCode);
        VkShaderModule fragModule = createShaderModule(fragCode);

        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertModule;
        vertStage.pName = "main";
        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragModule;
        fragStage.pName = "main";
        VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

        auto bindingDesc = Vertex::getBindingDescription();
        auto attrDescs = Vertex::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &bindingDesc;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
        vertexInput.pVertexAttributeDescriptions = attrDescs.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");

        // Wireframe pipeline for highlight
        VkPipelineRasterizationStateCreateInfo wireframeRasterizer = rasterizer;
        wireframeRasterizer.polygonMode = VK_POLYGON_MODE_LINE;

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline!");

        pipelineInfo.pRasterizationState = &wireframeRasterizer;
        VkPipelineInputAssemblyStateCreateInfo lineInputAssembly{};
        lineInputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        lineInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        lineInputAssembly.primitiveRestartEnable = VK_FALSE;
        pipelineInfo.pInputAssemblyState = &lineInputAssembly;
        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &linePipeline) != VK_SUCCESS)
            throw std::runtime_error("Failed to create line pipeline!");

        vkDestroyShaderModule(device, fragModule, nullptr);
        vkDestroyShaderModule(device, vertModule, nullptr);
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());
        // Create depth image
        VkImageCreateInfo depthImgInfo{};
        depthImgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        depthImgInfo.imageType = VK_IMAGE_TYPE_2D;
        depthImgInfo.extent = {swapChainExtent.width, swapChainExtent.height, 1};
        depthImgInfo.mipLevels = 1;
        depthImgInfo.arrayLayers = 1;
        depthImgInfo.format = depthFormat;
        depthImgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        depthImgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthImgInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthImgInfo.samples = VK_SAMPLE_COUNT_1_BIT;

        vkCreateImage(device, &depthImgInfo, nullptr, &depthImage);
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device, depthImage, &memReqs);
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        allocInfo.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(device, &allocInfo, nullptr, &depthImageMemory);
        vkBindImageMemory(device, depthImage, depthImageMemory, 0);

        VkImageViewCreateInfo depthViewInfo{};
        depthViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        depthViewInfo.image = depthImage;
        depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthViewInfo.format = depthFormat;
        depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthViewInfo.subresourceRange.baseMipLevel = 0;
        depthViewInfo.subresourceRange.levelCount = 1;
        depthViewInfo.subresourceRange.baseArrayLayer = 0;
        depthViewInfo.subresourceRange.layerCount = 1;
        vkCreateImageView(device, &depthViewInfo, nullptr, &depthImageView);

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            std::array<VkImageView, 2> attachments = {swapChainImageViews[i], depthImageView};
            VkFramebufferCreateInfo fi{};
            fi.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fi.renderPass = renderPass;
            fi.attachmentCount = static_cast<uint32_t>(attachments.size());
            fi.pAttachments = attachments.data();
            fi.width = swapChainExtent.width;
            fi.height = swapChainExtent.height;
            fi.layers = 1;
            if (vkCreateFramebuffer(device, &fi, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create framebuffer!");
        }
    }

    void createCommandPool() {
        auto indices = findQueueFamilies(physicalDevice);
        VkCommandPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        ci.queueFamilyIndex = indices.graphicsFamily.value();
        if (vkCreateCommandPool(device, &ci, nullptr, &commandPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create command pool!");
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags props) {
        VkPhysicalDeviceMemoryProperties memProps;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++)
            if ((typeFilter & (1 << i)) && (memProps.memoryTypes[i].propertyFlags & props) == props)
                return i;
        throw std::runtime_error("Failed to find suitable memory type!");
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props,
                      VkBuffer& buffer, VkDeviceMemory& memory) {
        VkBufferCreateInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size = size;
        bi.usage = usage;
        bi.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (vkCreateBuffer(device, &bi, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create buffer!");
        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(device, buffer, &memReqs);
        VkMemoryAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = memReqs.size;
        ai.memoryTypeIndex = findMemoryType(memReqs.memoryTypeBits, props);
        if (vkAllocateMemory(device, &ai, nullptr, &memory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate buffer memory!");
        vkBindBufferMemory(device, buffer, memory, 0);
    }

    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandPool = commandPool;
        ai.commandBufferCount = 1;
        VkCommandBuffer cb;
        vkAllocateCommandBuffers(device, &ai, &cb);
        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(cb, &bi);
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(cb, src, dst, 1, &copyRegion);
        vkEndCommandBuffer(cb);
        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &cb;
        vkQueueSubmit(graphicsQueue, 1, &si, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);
        vkFreeCommandBuffers(device, commandPool, 1, &cb);
    }

    // ==================== MESH ====================

    void generateMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        vertices.clear();
        indices.clear();

        glm::vec3 faceVerts[6][4] = {
            // +X
            {glm::vec3(1,0,0), glm::vec3(1,1,0), glm::vec3(1,1,1), glm::vec3(1,0,1)},
            // -X
            {glm::vec3(0,0,1), glm::vec3(0,1,1), glm::vec3(0,1,0), glm::vec3(0,0,0)},
            // +Y
            {glm::vec3(0,1,0), glm::vec3(0,1,1), glm::vec3(1,1,1), glm::vec3(1,1,0)},
            // -Y
            {glm::vec3(0,0,1), glm::vec3(0,0,0), glm::vec3(1,0,0), glm::vec3(1,0,1)},
            // +Z
            {glm::vec3(0,0,1), glm::vec3(0,1,1), glm::vec3(1,1,1), glm::vec3(1,0,1)},
            // -Z
            {glm::vec3(1,0,0), glm::vec3(1,1,0), glm::vec3(0,1,0), glm::vec3(0,0,0)}
        };

        glm::vec3 faceNormals[6] = {
            glm::vec3(1,0,0), glm::vec3(-1,0,0), glm::vec3(0,1,0),
            glm::vec3(0,-1,0), glm::vec3(0,0,1), glm::vec3(0,0,-1)
        };

        glm::ivec3 faceNeighbors[6] = {
            glm::ivec3(1,0,0), glm::ivec3(-1,0,0), glm::ivec3(0,1,0),
            glm::ivec3(0,-1,0), glm::ivec3(0,0,1), glm::ivec3(0,0,-1)
        };

        float darkMul[6] = {0.85f, 0.85f, 1.0f, 0.6f, 0.9f, 0.9f};

        for (int x = 0; x < WORLD_SIZE; x++) {
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                for (int z = 0; z < WORLD_SIZE; z++) {
                    BlockType block = world.get(x, y, z);
                    if (block == BLOCK_AIR) continue;

                    BlockColor color = getBlockColor(block);

                    for (int f = 0; f < 6; f++) {
                        glm::ivec3 n = glm::ivec3(x, y, z) + faceNeighbors[f];
                        if (world.get(n.x, n.y, n.z) != BLOCK_AIR) continue;

                        uint32_t base = static_cast<uint32_t>(vertices.size());
                        float dm = darkMul[f];
                        for (int v = 0; v < 4; v++) {
                            vertices.push_back({
                                faceVerts[f][v] + glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)),
                                glm::vec3(color.r * dm, color.g * dm, color.b * dm),
                                faceNormals[f]
                            });
                        }
                        indices.push_back(base + 0);
                        indices.push_back(base + 1);
                        indices.push_back(base + 2);
                        indices.push_back(base + 2);
                        indices.push_back(base + 3);
                        indices.push_back(base + 0);
                    }
                }
            }
        }
    }

    void createVertexBuffer() {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        generateMesh(vertices, indices);

        if (vertices.empty()) {
            vertices.push_back({{0,0,0},{1,0,1},{0,1,0}});
            indices.push_back(0);
        }

        VkDeviceSize vsize = sizeof(Vertex) * vertices.size();
        VkBuffer stagingVB;
        VkDeviceMemory stagingVM;
        createBuffer(vsize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingVB, stagingVM);
        void* data;
        vkMapMemory(device, stagingVM, 0, vsize, 0, &data);
        memcpy(data, vertices.data(), vsize);
        vkUnmapMemory(device, stagingVM);

        createBuffer(vsize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexMemory);
        copyBuffer(stagingVB, vertexBuffer, vsize);
        vkDestroyBuffer(device, stagingVB, nullptr);
        vkFreeMemory(device, stagingVM, nullptr);

        indexCount = static_cast<uint32_t>(indices.size());
        VkDeviceSize isize = sizeof(uint32_t) * indices.size();
        VkBuffer stagingIB;
        VkDeviceMemory stagingIM;
        createBuffer(isize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     stagingIB, stagingIM);
        vkMapMemory(device, stagingIM, 0, isize, 0, &data);
        memcpy(data, indices.data(), isize);
        vkUnmapMemory(device, stagingIM);

        createBuffer(isize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexMemory);
        copyBuffer(stagingIB, indexBuffer, isize);
        vkDestroyBuffer(device, stagingIB, nullptr);
        vkFreeMemory(device, stagingIM, nullptr);
    }

    void createHighlightBuffers() {
        float s = 1.005f; // slightly larger
        float o = -0.0025f;
        std::vector<Vertex> v = {
            {{o, o, o}, {1,1,1}, {0,1,0}}, {{s, o, o}, {1,1,1}, {0,1,0}},
            {{s, s, o}, {1,1,1}, {0,1,0}}, {{o, s, o}, {1,1,1}, {0,1,0}},
            {{o, o, s}, {1,1,1}, {0,1,0}}, {{s, o, s}, {1,1,1}, {0,1,0}},
            {{s, s, s}, {1,1,1}, {0,1,0}}, {{o, s, s}, {1,1,1}, {0,1,0}}
        };
        std::vector<uint32_t> idx = {
            0,1,1,2,2,3,3,0, 4,5,5,6,6,7,7,4,
            0,4,1,5,2,6,3,7
        };
        highlightIndexCount = static_cast<uint32_t>(idx.size());

        VkDeviceSize vs = sizeof(Vertex) * v.size();
        createBuffer(vs, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, highlightVB, highlightVM);
        void* d; vkMapMemory(device, highlightVM, 0, vs, 0, &d); memcpy(d, v.data(), vs); vkUnmapMemory(device, highlightVM);

        VkDeviceSize is = sizeof(uint32_t) * idx.size();
        createBuffer(is, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, highlightIB, highlightIM);
        vkMapMemory(device, highlightIM, 0, is, 0, &d); memcpy(d, idx.data(), is); vkUnmapMemory(device, highlightIM);
    }

    void createUniformBuffers() {
        VkDeviceSize size = sizeof(UBO);
        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         uniformBuffers[i], uniformBuffersMemory[i]);
            vkMapMemory(device, uniformBuffersMemory[i], 0, size, 0, &uniformBuffersMapped[i]);
        }
    }

    void createDescriptorPool() {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        VkDescriptorPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ci.poolSizeCount = 1;
        ci.pPoolSizes = &poolSize;
        ci.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        if (vkCreateDescriptorPool(device, &ci, nullptr, &descriptorPool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool!");
    }

    void createDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
        VkDescriptorSetAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool = descriptorPool;
        ai.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        ai.pSetLayouts = layouts.data();
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(device, &ai, descriptorSets.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate descriptor sets!");
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufInfo{};
            bufInfo.buffer = uniformBuffers[i];
            bufInfo.offset = 0;
            bufInfo.range = sizeof(UBO);
            VkWriteDescriptorSet w{};
            w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w.dstSet = descriptorSets[i];
            w.dstBinding = 0;
            w.dstArrayElement = 0;
            w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            w.descriptorCount = 1;
            w.pBufferInfo = &bufInfo;
            vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        VkCommandBufferAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ai.commandPool = commandPool;
        ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        ai.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
        if (vkAllocateCommandBuffers(device, &ai, commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffers!");
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        VkSemaphoreCreateInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VkFenceCreateInfo fi{};
        fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &si, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &si, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fi, nullptr, &inFlightFences[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create sync objects!");
        }
    }

    // ==================== 2D HUD ====================

    void createHudPipeline() {
        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 0;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        VkDescriptorSetLayoutCreateInfo dsLayoutInfo{};
        dsLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        dsLayoutInfo.bindingCount = 1;
        dsLayoutInfo.pBindings = &uboBinding;
        vkCreateDescriptorSetLayout(device, &dsLayoutInfo, nullptr, &hudDescriptorSetLayout);

        auto vertCode = readFile("shaders/shader2d.vert.spv");
        auto fragCode = readFile("shaders/shader2d.frag.spv");
        VkShaderModule vertMod = createShaderModule(vertCode);
        VkShaderModule fragMod = createShaderModule(fragCode);

        VkPipelineShaderStageCreateInfo vs{};
        vs.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vs.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vs.module = vertMod;
        vs.pName = "main";
        VkPipelineShaderStageCreateInfo fs{};
        fs.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fs.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fs.module = fragMod;
        fs.pName = "main";
        VkPipelineShaderStageCreateInfo stages[] = {vs, fs};

        auto bindingDesc = Vertex2D::getBindingDescription();
        auto attrDescs = Vertex2D::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInput{};
        vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInput.vertexBindingDescriptionCount = 1;
        vertexInput.pVertexBindingDescriptions = &bindingDesc;
        vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
        vertexInput.pVertexAttributeDescriptions = attrDescs.data();

        VkPipelineInputAssemblyStateCreateInfo ia{};
        ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkViewport viewport{};
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.maxDepth = 1.0f;
        VkRect2D scissor{};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo vsState{};
        vsState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vsState.viewportCount = 1;
        vsState.pViewports = &viewport;
        vsState.scissorCount = 1;
        vsState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_NONE;

        VkPipelineMultisampleStateCreateInfo ms{};
        ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo ds{};
        ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.depthTestEnable = VK_FALSE;
        ds.depthWriteEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState cba{};
        cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo cb{};
        cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.attachmentCount = 1;
        cb.pAttachments = &cba;

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &hudDescriptorSetLayout;
        vkCreatePipelineLayout(device, &layoutInfo, nullptr, &hudPipelineLayout);

        VkGraphicsPipelineCreateInfo pi{};
        pi.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pi.stageCount = 2;
        pi.pStages = stages;
        pi.pVertexInputState = &vertexInput;
        pi.pInputAssemblyState = &ia;
        pi.pViewportState = &vsState;
        pi.pRasterizationState = &rasterizer;
        pi.pMultisampleState = &ms;
        pi.pDepthStencilState = &ds;
        pi.pColorBlendState = &cb;
        pi.layout = hudPipelineLayout;
        pi.renderPass = renderPass;
        pi.subpass = 0;
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pi, nullptr, &hudPipeline);

        vkDestroyShaderModule(device, fragMod, nullptr);
        vkDestroyShaderModule(device, vertMod, nullptr);
    }

    void createHudUniformBuffers() {
        VkDeviceSize size = sizeof(UBO2D);
        hudUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        hudUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        hudUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         hudUniformBuffers[i], hudUniformBuffersMemory[i]);
            vkMapMemory(device, hudUniformBuffersMemory[i], 0, size, 0, &hudUniformBuffersMapped[i]);
        }
    }

    void createHudDescriptorPool() {
        VkDescriptorPoolSize ps{};
        ps.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ps.descriptorCount = MAX_FRAMES_IN_FLIGHT;
        VkDescriptorPoolCreateInfo ci{};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ci.poolSizeCount = 1;
        ci.pPoolSizes = &ps;
        ci.maxSets = MAX_FRAMES_IN_FLIGHT;
        vkCreateDescriptorPool(device, &ci, nullptr, &hudDescriptorPool);
    }

    void createHudDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, hudDescriptorSetLayout);
        VkDescriptorSetAllocateInfo ai{};
        ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ai.descriptorPool = hudDescriptorPool;
        ai.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
        ai.pSetLayouts = layouts.data();
        hudDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        vkAllocateDescriptorSets(device, &ai, hudDescriptorSets.data());
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bi{};
            bi.buffer = hudUniformBuffers[i];
            bi.range = sizeof(UBO2D);
            VkWriteDescriptorSet w{};
            w.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            w.dstSet = hudDescriptorSets[i];
            w.dstBinding = 0;
            w.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            w.descriptorCount = 1;
            w.pBufferInfo = &bi;
            vkUpdateDescriptorSets(device, 1, &w, 0, nullptr);
        }
    }

    void updateHudUniformBuffer(uint32_t imageIndex) {
        UBO2D ubo{};
        ubo.proj = glm::ortho(0.0f, static_cast<float>(WIDTH), static_cast<float>(HEIGHT), 0.0f, -1.0f, 1.0f);
        memcpy(hudUniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));
    }

    void updateHudVertexBuffer() {
        std::vector<Vertex2D> verts;

        int slotSize = 44;
        int gap = 6;
        int totalW = BLOCK_COUNT * slotSize + (BLOCK_COUNT - 1) * gap;
        float startX = (WIDTH - totalW) / 2.0f;
        float startY = HEIGHT - 70.0f;

        auto quad = [&](float x, float y, float w, float h, float r, float g, float b) {
            uint32_t base = static_cast<uint32_t>(verts.size());
            verts.push_back({{x, y}, {r, g, b}});
            verts.push_back({{x + w, y}, {r, g, b}});
            verts.push_back({{x + w, y + h}, {r, g, b}});
            verts.push_back({{x, y + h}, {r, g, b}});
            verts.push_back({{x + w, y + h}, {r, g, b}});
            verts.push_back({{x, y}, {r, g, b}});
        };

        float barW = totalW + 20.0f;
        float barH = slotSize + 14.0f;
        float barX = startX - 10.0f;
        float barY = startY - 7.0f;
        quad(barX, barY, barW, barH, 0.15f, 0.15f, 0.15f);

        for (int i = 1; i < BLOCK_COUNT; i++) {
            float x = startX + (i - 1) * (slotSize + gap);
            float y = startY;
            BlockColor c = getBlockColor(static_cast<BlockType>(i));

            if (static_cast<int>(selectedBlock) == i) {
                float b = 3.0f;
                quad(x - b, y - b, slotSize + b * 2, slotSize + b * 2, 1.0f, 1.0f, 1.0f);
            }

            float pad = 3.0f;
            quad(x + pad, y + pad, slotSize - pad * 2, slotSize - pad * 2, c.r, c.g, c.b);
        }

        float cx = WIDTH / 2.0f;
        float cy = HEIGHT / 2.0f;
        float cr = 2.0f;
        quad(cx - cr, cy - cr, cr * 2, cr * 2, 1.0f, 1.0f, 1.0f);

        hudVertexCount = static_cast<uint32_t>(verts.size());
        if (hudVertexCount == 0) return;

        if (hudVB) {
            vkDestroyBuffer(device, hudVB, nullptr);
            vkFreeMemory(device, hudVM, nullptr);
        }
        VkDeviceSize size = sizeof(Vertex2D) * verts.size();
        createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     hudVB, hudVM);
        void* d;
        vkMapMemory(device, hudVM, 0, size, 0, &d);
        memcpy(d, verts.data(), size);
        vkUnmapMemory(device, hudVM);
    }

    // ==================== RAYCASTING ====================

    bool raycastBlock(glm::vec3 origin, glm::vec3 dir, float maxDist, glm::ivec3& hitPos, glm::ivec3& placePos) {
        glm::ivec3 pos = glm::ivec3(floor(origin.x), floor(origin.y), floor(origin.z));
        glm::ivec3 step(
            dir.x > 0 ? 1 : (dir.x < 0 ? -1 : 0),
            dir.y > 0 ? 1 : (dir.y < 0 ? -1 : 0),
            dir.z > 0 ? 1 : (dir.z < 0 ? -1 : 0)
        );
        glm::vec3 tDelta(
            dir.x != 0 ? std::abs(1.0f / dir.x) : 1e30f,
            dir.y != 0 ? std::abs(1.0f / dir.y) : 1e30f,
            dir.z != 0 ? std::abs(1.0f / dir.z) : 1e30f
        );
        glm::vec3 tMax(
            dir.x != 0 ? ((dir.x > 0 ? (pos.x + 1.0f - origin.x) : (origin.x - pos.x)) * tDelta.x) : 1e30f,
            dir.y != 0 ? ((dir.y > 0 ? (pos.y + 1.0f - origin.y) : (origin.y - pos.y)) * tDelta.y) : 1e30f,
            dir.z != 0 ? ((dir.z > 0 ? (pos.z + 1.0f - origin.z) : (origin.z - pos.z)) * tDelta.z) : 1e30f
        );

        placePos = pos;

        for (int i = 0; i < maxDist * 3; i++) {
            if (pos.x < 0 || pos.x >= WORLD_SIZE || pos.y < 0 || pos.y >= WORLD_HEIGHT || pos.z < 0 || pos.z >= WORLD_SIZE)
                return false;

            if (world.get(pos.x, pos.y, pos.z) != BLOCK_AIR) {
                hitPos = pos;
                return true;
            }

            placePos = pos;

            if (tMax.x < tMax.y) {
                if (tMax.x < tMax.z) {
                    pos.x += step.x;
                    tMax.x += tDelta.x;
                } else {
                    pos.z += step.z;
                    tMax.z += tDelta.z;
                }
            } else {
                if (tMax.y < tMax.z) {
                    pos.y += step.y;
                    tMax.y += tDelta.y;
                } else {
                    pos.z += step.z;
                    tMax.z += tDelta.z;
                }
            }
        }
        return false;
    }

    void breakBlock() {
        glm::ivec3 hit, place;
        if (raycastBlock(camera.position, camera.front(), 8.0f, hit, place)) {
            world.set(hit.x, hit.y, hit.z, BLOCK_AIR);
            meshDirty = true;
        }
    }

    void placeBlock() {
        glm::ivec3 hit, place;
        if (raycastBlock(camera.position, camera.front(), 8.0f, hit, place)) {
            if (place.x >= 0 && place.x < WORLD_SIZE &&
                place.y >= 0 && place.y < WORLD_HEIGHT &&
                place.z >= 0 && place.z < WORLD_SIZE) {
                // Don't place inside the player
                glm::ivec3 pp(camera.position.x, camera.position.y - Camera::eyeHeight, camera.position.z);
                if (place != pp && place != pp + glm::ivec3(0, 1, 0)) {
                    world.set(place.x, place.y, place.z, selectedBlock);
                    meshDirty = true;
                }
            }
        }
    }

    // ==================== RENDER ====================

    void updateUniformBuffer(uint32_t imageIndex) {
        auto now = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float>(now - startTime).count();

        UBO ubo{};
        ubo.model = glm::mat4(1.0f);
        ubo.view = camera.viewMatrix();
        ubo.proj = glm::perspective(
            glm::radians(camera.fov),
            swapChainExtent.width / static_cast<float>(swapChainExtent.height),
            0.1f, 200.0f
        );
        ubo.proj[1][1] *= -1;
        ubo.lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
        ubo.time = time;

        memcpy(uniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));
    }

    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
            imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        }

        vkResetFences(device, 1, &inFlightFences[currentFrame]);
        updateUniformBuffer(currentFrame);
        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        VkSubmitInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        si.waitSemaphoreCount = 1;
        si.pWaitSemaphores = waitSemaphores;
        si.pWaitDstStageMask = waitStages;
        si.commandBufferCount = 1;
        si.pCommandBuffers = &commandBuffers[currentFrame];
        si.signalSemaphoreCount = 1;
        si.pSignalSemaphores = signalSemaphores;
        if (vkQueueSubmit(graphicsQueue, 1, &si, inFlightFences[currentFrame]) != VK_SUCCESS)
            throw std::runtime_error("Failed to submit draw command!");

        VkPresentInfoKHR pi{};
        pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pi.waitSemaphoreCount = 1;
        pi.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapchains[] = {swapChain};
        pi.swapchainCount = 1;
        pi.pSwapchains = swapchains;
        pi.pImageIndices = &imageIndex;
        result = vkQueuePresentKHR(presentQueue, &pi);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void recordCommandBuffer(VkCommandBuffer cb, uint32_t imageIndex) {
        VkCommandBufferBeginInfo bi{};
        bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(cb, &bi);

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {{0.60f, 0.75f, 0.95f, 1.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        VkRenderPassBeginInfo rpbi{};
        rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpbi.renderPass = renderPass;
        rpbi.framebuffer = swapChainFramebuffers[imageIndex];
        rpbi.renderArea.offset = {0, 0};
        rpbi.renderArea.extent = swapChainExtent;
        rpbi.clearValueCount = static_cast<uint32_t>(clearValues.size());
        rpbi.pClearValues = clearValues.data();
        vkCmdBeginRenderPass(cb, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        VkViewport viewport{};
        viewport.x = 0.0f; viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cb, 0, 1, &viewport);
        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(cb, 0, 1, &scissor);

        vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                &descriptorSets[currentFrame], 0, nullptr);

        if (indexCount > 0) {
            VkBuffer vb[] = {vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cb, 0, 1, vb, offsets);
            vkCmdBindIndexBuffer(cb, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cb, indexCount, 1, 0, 0, 0);
        }

        // Draw highlight
        if (highlightedBlock.x >= 0) {
            vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline);
            vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                                    &descriptorSets[currentFrame], 0, nullptr);
            VkBuffer hvb[] = {highlightVB};
            VkDeviceSize hoff[] = {0};
            vkCmdBindVertexBuffers(cb, 0, 1, hvb, hoff);
            vkCmdBindIndexBuffer(cb, highlightIB, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(cb, highlightIndexCount, 1, 0, 0, 0);
            vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        }

        // Draw 2D HUD
        updateHudVertexBuffer();
        updateHudUniformBuffer(currentFrame);
        if (hudVertexCount > 0) {
            vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, hudPipeline);
            vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, hudPipelineLayout, 0, 1,
                                    &hudDescriptorSets[currentFrame], 0, nullptr);
            VkBuffer vbs[] = {hudVB};
            VkDeviceSize off[] = {0};
            vkCmdBindVertexBuffers(cb, 0, 1, vbs, off);
            vkCmdDraw(cb, hudVertexCount, 1, 0, 0);
            vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        }

        vkCmdEndRenderPass(cb);
        if (vkEndCommandBuffer(cb) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer!");
    }

    void recreateSwapChain() {
        int w = 0, h = 0;
        glfwGetFramebufferSize(window, &w, &h);
        while (w == 0 || h == 0) {
            glfwGetFramebufferSize(window, &w, &h);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(device);
        cleanupSwapChain();
        createSwapChain();
        createImageViews();
        createFramebuffers();
    }

    void cleanupSwapChain() {
        vkDestroyImageView(device, depthImageView, nullptr);
        vkDestroyImage(device, depthImage, nullptr);
        vkFreeMemory(device, depthImageMemory, nullptr);
        for (auto fb : swapChainFramebuffers) vkDestroyFramebuffer(device, fb, nullptr);
        for (auto iv : swapChainImageViews) vkDestroyImageView(device, iv, nullptr);
        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    // ==================== MAIN LOOP ====================

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            processInput();

            // Raycast for highlight
            glm::ivec3 hit, place;
            if (raycastBlock(camera.position, camera.front(), 8.0f, hit, place)) {
                highlightedBlock = hit;
                // Update highlight buffer position
                updateHighlightPosition(hit);
            } else {
                highlightedBlock = {-1, -1, -1};
            }

            if (meshDirty) {
                vkDeviceWaitIdle(device);
                vkDestroyBuffer(device, vertexBuffer, nullptr);
                vkFreeMemory(device, vertexMemory, nullptr);
                vkDestroyBuffer(device, indexBuffer, nullptr);
                vkFreeMemory(device, indexMemory, nullptr);
                createVertexBuffer();
                meshDirty = false;
            }

            drawFrame();
            glfwPollEvents();
        }
        vkDeviceWaitIdle(device);
    }

    void updateHighlightPosition(glm::ivec3 pos) {
        float s = 1.005f;
        float o = -0.0025f;
        std::vector<Vertex> v = {
            {{pos.x+o, pos.y+o, pos.z+o}, {1,1,1}, {0,1,0}}, {{pos.x+s, pos.y+o, pos.z+o}, {1,1,1}, {0,1,0}},
            {{pos.x+s, pos.y+s, pos.z+o}, {1,1,1}, {0,1,0}}, {{pos.x+o, pos.y+s, pos.z+o}, {1,1,1}, {0,1,0}},
            {{pos.x+o, pos.y+o, pos.z+s}, {1,1,1}, {0,1,0}}, {{pos.x+s, pos.y+o, pos.z+s}, {1,1,1}, {0,1,0}},
            {{pos.x+s, pos.y+s, pos.z+s}, {1,1,1}, {0,1,0}}, {{pos.x+o, pos.y+s, pos.z+s}, {1,1,1}, {0,1,0}}
        };
        VkDeviceSize vs = sizeof(Vertex) * v.size();
        void* d;
        vkMapMemory(device, highlightVM, 0, vs, 0, &d);
        memcpy(d, v.data(), vs);
        vkUnmapMemory(device, highlightVM);
    }

    void processInput() {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processMovement(0, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processMovement(1, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processMovement(2, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processMovement(3, deltaTime);

        bool spaceDown = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool shiftDown = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

        if (camera.flying) {
            if (spaceDown) camera.processMovement(4, deltaTime);
            if (shiftDown) camera.processMovement(5, deltaTime);

            int bx = static_cast<int>(floor(camera.position.x));
            int bz = static_cast<int>(floor(camera.position.z));
            int by = static_cast<int>(floor(camera.position.y));
            for (int y = by; y >= 0; y--) {
                if (world.get(bx, y, bz) != BLOCK_AIR) {
                    if (camera.position.y - (y + 1) < 1.5f && shiftDown) {
                        camera.flying = false;
                        camera.velocityY = 0;
                    }
                    break;
                }
            }
        } else {
            if (spaceDown && camera.onGround && !camera.spaceWasPressed) {
                camera.velocityY = Camera::jumpVelocity;
                camera.onGround = false;
            } else if (spaceDown && !camera.onGround && !camera.spaceWasPressed) {
                camera.flying = true;
                camera.velocityY = 0;
            }
        }
        camera.spaceWasPressed = spaceDown;

        applyPhysics(camera, world, deltaTime);
    }

    // ==================== CLEANUP ====================

    void cleanup() {
        cleanupSwapChain();

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexMemory, nullptr);
        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexMemory, nullptr);

        vkDestroyBuffer(device, highlightIB, nullptr);
        vkFreeMemory(device, highlightIM, nullptr);
        vkDestroyBuffer(device, highlightVB, nullptr);
        vkFreeMemory(device, highlightVM, nullptr);

        vkDestroyBuffer(device, hudVB, nullptr);
        vkFreeMemory(device, hudVM, nullptr);
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(device, hudUniformBuffers[i], nullptr);
            vkFreeMemory(device, hudUniformBuffersMemory[i], nullptr);
        }
        vkDestroyDescriptorPool(device, hudDescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, hudDescriptorSetLayout, nullptr);
        vkDestroyPipeline(device, hudPipeline, nullptr);
        vkDestroyPipelineLayout(device, hudPipelineLayout, nullptr);

        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipeline(device, linePipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);

        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
            vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
            vkDestroyFence(device, inFlightFences[i], nullptr);
        }
        vkDestroyCommandPool(device, commandPool, nullptr);
        vkDestroyDevice(device, nullptr);
        vkDestroySurfaceKHR(instance, surface, nullptr);
        vkDestroyInstance(instance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

int main() {
    try {
        VoxelEngine engine;
        engine.run();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
