#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
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
    BLOCK_DOOR,
    BLOCK_GLASS,
    BLOCK_COBBLESTONE,
    BLOCK_PLANKS,
    BLOCK_STONE_BRICKS,
    BLOCK_SNOW,
    BLOCK_ICE,
    BLOCK_NETHERRACK,
    BLOCK_OBSIDIAN,
    BLOCK_GOLD,
    BLOCK_IRON,
    BLOCK_DIAMOND,
    BLOCK_LAPIS,
    BLOCK_EMERALD,
    BLOCK_REDSTONE,
    BLOCK_SMOOTH_STONE,
    BLOCK_MOSSY_COBBLESTONE,
    BLOCK_BEDROCK,
    BLOCK_COUNT
};

enum BuildMode : int { BUILD_BLOCK = 0, BUILD_SLAB = 1, BUILD_STAIR = 2 };

struct BlockColor {
    float r, g, b, a;
    BlockColor(float r, float g, float b, float a = 1.0f) : r(r), g(g), b(b), a(a) {}
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
        case BLOCK_DOOR:   return {0.40f, 0.22f, 0.12f};
        case BLOCK_GLASS:  return {0.60f, 0.85f, 1.00f, 0.45f};
        case BLOCK_COBBLESTONE: return {0.45f, 0.45f, 0.45f};
        case BLOCK_PLANKS: return {0.65f, 0.50f, 0.30f};
        case BLOCK_STONE_BRICKS: return {0.55f, 0.55f, 0.55f};
        case BLOCK_SNOW:   return {0.95f, 0.95f, 0.98f};
        case BLOCK_ICE:    return {0.70f, 0.85f, 1.00f, 0.60f};
        case BLOCK_NETHERRACK: return {0.50f, 0.15f, 0.15f};
        case BLOCK_OBSIDIAN: return {0.10f, 0.08f, 0.18f};
        case BLOCK_GOLD:   return {1.00f, 0.85f, 0.10f};
        case BLOCK_IRON:   return {0.85f, 0.85f, 0.88f};
        case BLOCK_DIAMOND: return {0.20f, 0.80f, 0.80f};
        case BLOCK_LAPIS:  return {0.15f, 0.30f, 0.70f};
        case BLOCK_EMERALD: return {0.20f, 0.75f, 0.30f};
        case BLOCK_REDSTONE: return {0.75f, 0.10f, 0.10f};
        case BLOCK_SMOOTH_STONE: return {0.60f, 0.60f, 0.62f};
        case BLOCK_MOSSY_COBBLESTONE: return {0.40f, 0.50f, 0.35f};
        case BLOCK_BEDROCK: return {0.20f, 0.20f, 0.20f};
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
        case BLOCK_DOOR:   return "Door";
        case BLOCK_GLASS:  return "Glass";
        case BLOCK_COBBLESTONE: return "Cobblestone";
        case BLOCK_PLANKS: return "Planks";
        case BLOCK_STONE_BRICKS: return "Stone Bricks";
        case BLOCK_SNOW:   return "Snow";
        case BLOCK_ICE:    return "Ice";
        case BLOCK_NETHERRACK: return "Netherrack";
        case BLOCK_OBSIDIAN: return "Obsidian";
        case BLOCK_GOLD:   return "Gold Block";
        case BLOCK_IRON:   return "Iron Block";
        case BLOCK_DIAMOND: return "Diamond Block";
        case BLOCK_LAPIS:  return "Lapis Block";
        case BLOCK_EMERALD: return "Emerald Block";
        case BLOCK_REDSTONE: return "Redstone Block";
        case BLOCK_SMOOTH_STONE: return "Smooth Stone";
        case BLOCK_MOSSY_COBBLESTONE: return "Mossy Cobblestone";
        case BLOCK_BEDROCK: return "Bedrock";
        default:           return "Unknown";
    }
}

struct Vertex {
    glm::vec3 pos;
    glm::vec4 color;
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

// 5x7 bitmap font: each char = 7 bytes (rows 0-6), bits 0-4 = columns 0-4
static uint8_t fontData[128][7] = {0};
static void initFont() {
    // Each comment shows ASCII index and character
    fontData[32][0] = 0; // space (all zeros)
    fontData[33][0] = 0x04; fontData[33][1]=0x04; fontData[33][2]=0x04; fontData[33][3]=0x04; fontData[33][4]=0x04; fontData[33][5]=0x00; fontData[33][6]=0x04; // !
    fontData[44][0] = 0x0C; fontData[44][1]=0x0C; // ,
    fontData[45][0] = 0x00; fontData[45][1]=0x00; fontData[45][2]=0x00; fontData[45][3]=0x1F; fontData[45][4]=0x00; fontData[45][5]=0x00; fontData[45][6]=0x00; // -
    fontData[46][0] = 0x00; fontData[46][1]=0x00; fontData[46][2]=0x00; fontData[46][3]=0x00; fontData[46][4]=0x00; fontData[46][5]=0x0C; fontData[46][6]=0x0C; // .
    fontData[47][0] = 0x01; fontData[47][1]=0x02; fontData[47][2]=0x02; fontData[47][3]=0x04; fontData[47][4]=0x04; fontData[47][5]=0x08; fontData[47][6]=0x08; // /
    // 0-9
    fontData[48][0]=0x0E; fontData[48][1]=0x11; fontData[48][2]=0x11; fontData[48][3]=0x11; fontData[48][4]=0x11; fontData[48][5]=0x11; fontData[48][6]=0x0E;
    fontData[49][0]=0x04; fontData[49][1]=0x0C; fontData[49][2]=0x04; fontData[49][3]=0x04; fontData[49][4]=0x04; fontData[49][5]=0x04; fontData[49][6]=0x0E;
    fontData[50][0]=0x0E; fontData[50][1]=0x11; fontData[50][2]=0x01; fontData[50][3]=0x02; fontData[50][4]=0x04; fontData[50][5]=0x08; fontData[50][6]=0x1F;
    fontData[51][0]=0x0E; fontData[51][1]=0x11; fontData[51][2]=0x01; fontData[51][3]=0x06; fontData[51][4]=0x01; fontData[51][5]=0x11; fontData[51][6]=0x0E;
    fontData[52][0]=0x02; fontData[52][1]=0x06; fontData[52][2]=0x0A; fontData[52][3]=0x12; fontData[52][4]=0x1F; fontData[52][5]=0x02; fontData[52][6]=0x02;
    fontData[53][0]=0x1F; fontData[53][1]=0x10; fontData[53][2]=0x1E; fontData[53][3]=0x01; fontData[53][4]=0x01; fontData[53][5]=0x11; fontData[53][6]=0x0E;
    fontData[54][0]=0x06; fontData[54][1]=0x08; fontData[54][2]=0x10; fontData[54][3]=0x1E; fontData[54][4]=0x11; fontData[54][5]=0x11; fontData[54][6]=0x0E;
    fontData[55][0]=0x1F; fontData[55][1]=0x01; fontData[55][2]=0x02; fontData[55][3]=0x04; fontData[55][4]=0x04; fontData[55][5]=0x04; fontData[55][6]=0x04;
    fontData[56][0]=0x0E; fontData[56][1]=0x11; fontData[56][2]=0x11; fontData[56][3]=0x0E; fontData[56][4]=0x11; fontData[56][5]=0x11; fontData[56][6]=0x0E;
    fontData[57][0]=0x0E; fontData[57][1]=0x11; fontData[57][2]=0x11; fontData[57][3]=0x0F; fontData[57][4]=0x01; fontData[57][5]=0x02; fontData[57][6]=0x0C;
    // : ; < = > ? @
    fontData[58][0]=0x0C; fontData[58][1]=0x0C; fontData[58][2]=0x00; fontData[58][3]=0x00; fontData[58][4]=0x00; fontData[58][5]=0x0C; fontData[58][6]=0x0C; // :
    // A-Z
    fontData[65][0]=0x0E; fontData[65][1]=0x11; fontData[65][2]=0x11; fontData[65][3]=0x1F; fontData[65][4]=0x11; fontData[65][5]=0x11; fontData[65][6]=0x11;
    fontData[66][0]=0x1E; fontData[66][1]=0x11; fontData[66][2]=0x11; fontData[66][3]=0x1E; fontData[66][4]=0x11; fontData[66][5]=0x11; fontData[66][6]=0x1E;
    fontData[67][0]=0x0E; fontData[67][1]=0x11; fontData[67][2]=0x10; fontData[67][3]=0x10; fontData[67][4]=0x10; fontData[67][5]=0x11; fontData[67][6]=0x0E;
    fontData[68][0]=0x1E; fontData[68][1]=0x11; fontData[68][2]=0x11; fontData[68][3]=0x11; fontData[68][4]=0x11; fontData[68][5]=0x11; fontData[68][6]=0x1E;
    fontData[69][0]=0x1F; fontData[69][1]=0x10; fontData[69][2]=0x10; fontData[69][3]=0x1E; fontData[69][4]=0x10; fontData[69][5]=0x10; fontData[69][6]=0x1F;
    fontData[70][0]=0x1F; fontData[70][1]=0x10; fontData[70][2]=0x10; fontData[70][3]=0x1E; fontData[70][4]=0x10; fontData[70][5]=0x10; fontData[70][6]=0x10;
    fontData[71][0]=0x0E; fontData[71][1]=0x11; fontData[71][2]=0x10; fontData[71][3]=0x17; fontData[71][4]=0x11; fontData[71][5]=0x11; fontData[71][6]=0x0F;
    fontData[72][0]=0x11; fontData[72][1]=0x11; fontData[72][2]=0x11; fontData[72][3]=0x1F; fontData[72][4]=0x11; fontData[72][5]=0x11; fontData[72][6]=0x11;
    fontData[73][0]=0x0E; fontData[73][1]=0x04; fontData[73][2]=0x04; fontData[73][3]=0x04; fontData[73][4]=0x04; fontData[73][5]=0x04; fontData[73][6]=0x0E;
    fontData[74][0]=0x01; fontData[74][1]=0x01; fontData[74][2]=0x01; fontData[74][3]=0x01; fontData[74][4]=0x01; fontData[74][5]=0x11; fontData[74][6]=0x0E;
    fontData[75][0]=0x11; fontData[75][1]=0x12; fontData[75][2]=0x14; fontData[75][3]=0x18; fontData[75][4]=0x14; fontData[75][5]=0x12; fontData[75][6]=0x11;
    fontData[76][0]=0x10; fontData[76][1]=0x10; fontData[76][2]=0x10; fontData[76][3]=0x10; fontData[76][4]=0x10; fontData[76][5]=0x10; fontData[76][6]=0x1F;
    fontData[77][0]=0x11; fontData[77][1]=0x1B; fontData[77][2]=0x15; fontData[77][3]=0x11; fontData[77][4]=0x11; fontData[77][5]=0x11; fontData[77][6]=0x11;
    fontData[78][0]=0x11; fontData[78][1]=0x19; fontData[78][2]=0x15; fontData[78][3]=0x13; fontData[78][4]=0x11; fontData[78][5]=0x11; fontData[78][6]=0x11;
    fontData[79][0]=0x0E; fontData[79][1]=0x11; fontData[79][2]=0x11; fontData[79][3]=0x11; fontData[79][4]=0x11; fontData[79][5]=0x11; fontData[79][6]=0x0E;
    fontData[80][0]=0x1E; fontData[80][1]=0x11; fontData[80][2]=0x11; fontData[80][3]=0x1E; fontData[80][4]=0x10; fontData[80][5]=0x10; fontData[80][6]=0x10;
    fontData[81][0]=0x0E; fontData[81][1]=0x11; fontData[81][2]=0x11; fontData[81][3]=0x11; fontData[81][4]=0x15; fontData[81][5]=0x12; fontData[81][6]=0x0D;
    fontData[82][0]=0x1E; fontData[82][1]=0x11; fontData[82][2]=0x11; fontData[82][3]=0x1E; fontData[82][4]=0x14; fontData[82][5]=0x12; fontData[82][6]=0x11;
    fontData[83][0]=0x0F; fontData[83][1]=0x10; fontData[83][2]=0x10; fontData[83][3]=0x0E; fontData[83][4]=0x01; fontData[83][5]=0x01; fontData[83][6]=0x1E;
    fontData[84][0]=0x1F; fontData[84][1]=0x04; fontData[84][2]=0x04; fontData[84][3]=0x04; fontData[84][4]=0x04; fontData[84][5]=0x04; fontData[84][6]=0x04;
    fontData[85][0]=0x11; fontData[85][1]=0x11; fontData[85][2]=0x11; fontData[85][3]=0x11; fontData[85][4]=0x11; fontData[85][5]=0x11; fontData[85][6]=0x0E;
    fontData[86][0]=0x11; fontData[86][1]=0x11; fontData[86][2]=0x11; fontData[86][3]=0x11; fontData[86][4]=0x11; fontData[86][5]=0x0A; fontData[86][6]=0x04;
    fontData[87][0]=0x11; fontData[87][1]=0x11; fontData[87][2]=0x11; fontData[87][3]=0x15; fontData[87][4]=0x15; fontData[87][5]=0x1B; fontData[87][6]=0x11;
    fontData[88][0]=0x11; fontData[88][1]=0x11; fontData[88][2]=0x0A; fontData[88][3]=0x04; fontData[88][4]=0x0A; fontData[88][5]=0x11; fontData[88][6]=0x11;
    fontData[89][0]=0x11; fontData[89][1]=0x11; fontData[89][2]=0x0A; fontData[89][3]=0x04; fontData[89][4]=0x04; fontData[89][5]=0x04; fontData[89][6]=0x04;
    fontData[90][0]=0x1F; fontData[90][1]=0x01; fontData[90][2]=0x02; fontData[90][3]=0x04; fontData[90][4]=0x08; fontData[90][5]=0x10; fontData[90][6]=0x1F;
    // _
    fontData[95][0]=0x00; fontData[95][1]=0x00; fontData[95][2]=0x00; fontData[95][3]=0x00; fontData[95][4]=0x00; fontData[95][5]=0x00; fontData[95][6]=0x1F;
    // a-z
    fontData[97][0]=0x00; fontData[97][1]=0x00; fontData[97][2]=0x0E; fontData[97][3]=0x01; fontData[97][4]=0x0F; fontData[97][5]=0x11; fontData[97][6]=0x0F;
    fontData[98][0]=0x10; fontData[98][1]=0x10; fontData[98][2]=0x1E; fontData[98][3]=0x11; fontData[98][4]=0x11; fontData[98][5]=0x11; fontData[98][6]=0x1E;
    fontData[99][0]=0x00; fontData[99][1]=0x00; fontData[99][2]=0x0E; fontData[99][3]=0x11; fontData[99][4]=0x10; fontData[99][5]=0x11; fontData[99][6]=0x0E;
    fontData[100][0]=0x01; fontData[100][1]=0x01; fontData[100][2]=0x0F; fontData[100][3]=0x11; fontData[100][4]=0x11; fontData[100][5]=0x11; fontData[100][6]=0x0F;
    fontData[101][0]=0x00; fontData[101][1]=0x00; fontData[101][2]=0x0E; fontData[101][3]=0x11; fontData[101][4]=0x1F; fontData[101][5]=0x10; fontData[101][6]=0x0E;
    fontData[102][0]=0x06; fontData[102][1]=0x09; fontData[102][2]=0x08; fontData[102][3]=0x1C; fontData[102][4]=0x08; fontData[102][5]=0x08; fontData[102][6]=0x08;
    fontData[103][0]=0x00; fontData[103][1]=0x00; fontData[103][2]=0x0F; fontData[103][3]=0x11; fontData[103][4]=0x11; fontData[103][5]=0x0F; fontData[103][6]=0x01;
    fontData[104][0]=0x10; fontData[104][1]=0x10; fontData[104][2]=0x1E; fontData[104][3]=0x11; fontData[104][4]=0x11; fontData[104][5]=0x11; fontData[104][6]=0x11;
    fontData[105][0]=0x04; fontData[105][1]=0x00; fontData[105][2]=0x0C; fontData[105][3]=0x04; fontData[105][4]=0x04; fontData[105][5]=0x04; fontData[105][6]=0x0E;
    fontData[106][0]=0x02; fontData[106][1]=0x00; fontData[106][2]=0x06; fontData[106][3]=0x02; fontData[106][4]=0x02; fontData[106][5]=0x12; fontData[106][6]=0x0C;
    fontData[107][0]=0x10; fontData[107][1]=0x10; fontData[107][2]=0x12; fontData[107][3]=0x14; fontData[107][4]=0x18; fontData[107][5]=0x14; fontData[107][6]=0x12;
    fontData[108][0]=0x0C; fontData[108][1]=0x04; fontData[108][2]=0x04; fontData[108][3]=0x04; fontData[108][4]=0x04; fontData[108][5]=0x04; fontData[108][6]=0x0E;
    fontData[109][0]=0x00; fontData[109][1]=0x00; fontData[109][2]=0x1A; fontData[109][3]=0x15; fontData[109][4]=0x15; fontData[109][5]=0x15; fontData[109][6]=0x15;
    fontData[110][0]=0x00; fontData[110][1]=0x00; fontData[110][2]=0x1E; fontData[110][3]=0x11; fontData[110][4]=0x11; fontData[110][5]=0x11; fontData[110][6]=0x11;
    fontData[111][0]=0x00; fontData[111][1]=0x00; fontData[111][2]=0x0E; fontData[111][3]=0x11; fontData[111][4]=0x11; fontData[111][5]=0x11; fontData[111][6]=0x0E;
    fontData[112][0]=0x00; fontData[112][1]=0x00; fontData[112][2]=0x1E; fontData[112][3]=0x11; fontData[112][4]=0x11; fontData[112][5]=0x1E; fontData[112][6]=0x10;
    fontData[113][0]=0x00; fontData[113][1]=0x00; fontData[113][2]=0x0F; fontData[113][3]=0x11; fontData[113][4]=0x11; fontData[113][5]=0x0F; fontData[113][6]=0x01;
    fontData[114][0]=0x00; fontData[114][1]=0x00; fontData[114][2]=0x16; fontData[114][3]=0x19; fontData[114][4]=0x10; fontData[114][5]=0x10; fontData[114][6]=0x10;
    fontData[115][0]=0x00; fontData[115][1]=0x00; fontData[115][2]=0x0F; fontData[115][3]=0x10; fontData[115][4]=0x0E; fontData[115][5]=0x01; fontData[115][6]=0x1E;
    fontData[116][0]=0x08; fontData[116][1]=0x08; fontData[116][2]=0x1C; fontData[116][3]=0x08; fontData[116][4]=0x08; fontData[116][5]=0x09; fontData[116][6]=0x06;
    fontData[117][0]=0x00; fontData[117][1]=0x00; fontData[117][2]=0x11; fontData[117][3]=0x11; fontData[117][4]=0x11; fontData[117][5]=0x11; fontData[117][6]=0x0E;
    fontData[118][0]=0x00; fontData[118][1]=0x00; fontData[118][2]=0x11; fontData[118][3]=0x11; fontData[118][4]=0x11; fontData[118][5]=0x0A; fontData[118][6]=0x04;
    fontData[119][0]=0x00; fontData[119][1]=0x00; fontData[119][2]=0x11; fontData[119][3]=0x11; fontData[119][4]=0x15; fontData[119][5]=0x15; fontData[119][6]=0x0A;
    fontData[120][0]=0x00; fontData[120][1]=0x00; fontData[120][2]=0x11; fontData[120][3]=0x0A; fontData[120][4]=0x04; fontData[120][5]=0x0A; fontData[120][6]=0x11;
    fontData[121][0]=0x00; fontData[121][1]=0x00; fontData[121][2]=0x11; fontData[121][3]=0x11; fontData[121][4]=0x0A; fontData[121][5]=0x04; fontData[121][6]=0x08;
    fontData[122][0]=0x00; fontData[122][1]=0x00; fontData[122][2]=0x1F; fontData[122][3]=0x02; fontData[122][4]=0x04; fontData[122][5]=0x08; fontData[122][6]=0x1F;
}

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
    float eyeHeight = 1.6f;

    bool flying = false;
    bool crouching = false;
    float velocityY = 0.0f;
    bool onGround = false;
    bool spaceWasPressed = false;
    bool fWasPressed = false;
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
    uint8_t meta[WORLD_SIZE][WORLD_HEIGHT][WORLD_SIZE];
    bool doorOpen[WORLD_SIZE][WORLD_HEIGHT][WORLD_SIZE];

    void generate() {
        for (int x = 0; x < WORLD_SIZE; x++) {
            for (int z = 0; z < WORLD_SIZE; z++) {
                blocks[x][0][z] = BLOCK_STONE;
                for (int y = 1; y < 4; y++) blocks[x][y][z] = BLOCK_DIRT;
                blocks[x][4][z] = BLOCK_GRASS;
                for (int y = 5; y < WORLD_HEIGHT; y++) blocks[x][y][z] = BLOCK_AIR;
                for (int y = 0; y < WORLD_HEIGHT; y++) { meta[x][y][z] = 0; doorOpen[x][y][z] = false; }
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

    uint8_t getMeta(int x, int y, int z) const {
        if (x < 0 || x >= WORLD_SIZE || y < 0 || y >= WORLD_HEIGHT || z < 0 || z >= WORLD_SIZE) return 0;
        return meta[x][y][z];
    }

    void setMeta(int x, int y, int z, uint8_t v) {
        if (x >= 0 && x < WORLD_SIZE && y >= 0 && y < WORLD_HEIGHT && z >= 0 && z < WORLD_SIZE)
            meta[x][y][z] = v;
    }

    bool getDoorOpen(int x, int y, int z) const {
        if (x < 0 || x >= WORLD_SIZE || y < 0 || y >= WORLD_HEIGHT || z < 0 || z >= WORLD_SIZE) return false;
        return doorOpen[x][y][z];
    }

    void setDoorOpen(int x, int y, int z, bool v) {
        if (x >= 0 && x < WORLD_SIZE && y >= 0 && y < WORLD_HEIGHT && z >= 0 && z < WORLD_SIZE)
            doorOpen[x][y][z] = v;
    }
};

bool isPlayerColliding(const Camera& camera, const World& world);

inline void applyPhysics(Camera& camera, const World& world, float dt) {
    if (camera.flying) { camera.onGround = false; return; }

    camera.velocityY += Camera::gravity * dt;
    if (camera.velocityY < -50.0f) camera.velocityY = -50.0f;

    float dy = camera.velocityY * dt;
    float step = (dy < 0) ? std::max(dy, -0.15f) : std::min(dy, 0.15f);
    bool grounded = false;

    while (std::abs(dy) > 0.0001f) {
        float move = (std::abs(dy) < std::abs(step)) ? dy : step;
        camera.position.y += move;
        dy -= move;

        if (isPlayerColliding(camera, world)) {
            camera.position.y -= move;
            if (move < 0) grounded = true;
            camera.velocityY = 0;
            break;
        }
    }
    camera.onGround = grounded;
}

bool isPlayerColliding(const Camera& camera, const World& world) {
    float r = 0.25f;
    float feetY = camera.position.y - camera.eyeHeight;
    float headY = camera.position.y + (camera.crouching ? 0.0f : 0.2f);
    int minX = (int)floor(camera.position.x - r);
    int maxX = (int)floor(camera.position.x + r);
    int minZ = (int)floor(camera.position.z - r);
    int maxZ = (int)floor(camera.position.z + r);
    int minY = (int)floor(feetY);
    int maxY = (int)floor(headY);
    for (int x = minX; x <= maxX; x++)
        for (int z = minZ; z <= maxZ; z++)
            for (int y = minY; y <= maxY; y++) {
                BlockType bt = world.get(x, y, z);
                if (bt == BLOCK_AIR) continue;
                if (bt == BLOCK_DOOR && world.getDoorOpen(x, y, z)) continue;
                return true;
            }
    return false;
}

enum GameState { MAIN_MENU, PLAYING };

struct SaveInfo {
    std::string name;
};

static const char* SAVE_DIR = "saves";

std::vector<SaveInfo> listSaves() {
    std::vector<SaveInfo> saves;
    if (!std::filesystem::is_directory(SAVE_DIR)) return saves;
    for (auto& e : std::filesystem::directory_iterator(SAVE_DIR)) {
        if (e.path().extension() == ".vxl") {
            std::string name = e.path().stem().string();
            saves.push_back({name});
        }
    }
    std::sort(saves.begin(), saves.end(), [](auto& a, auto& b) {
        return a.name < b.name;
    });
    return saves;
}

bool saveWorld(const World& world, const Camera& camera, const std::string& name) {
    std::filesystem::create_directories(SAVE_DIR);
    std::string path = std::string(SAVE_DIR) + "/" + name + ".vxl";
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write("VOXELSAV", 8);
    uint32_t ver = 2;
    f.write((char*)&ver, sizeof(ver));
    int32_t ws = WORLD_SIZE, wh = WORLD_HEIGHT;
    f.write((char*)&ws, sizeof(ws));
    f.write((char*)&wh, sizeof(wh));
    f.write((char*)world.blocks, sizeof(world.blocks));
    f.write((char*)world.meta, sizeof(world.meta));
    f.write((char*)world.doorOpen, sizeof(world.doorOpen));
    f.write((char*)&camera.position, sizeof(camera.position));
    f.write((char*)&camera.yaw, sizeof(camera.yaw));
    f.write((char*)&camera.pitch, sizeof(camera.pitch));
    f.write((char*)&camera.flying, sizeof(camera.flying));
    return true;
}

bool loadWorld(World& world, Camera& camera, const std::string& name) {
    std::string path = std::string(SAVE_DIR) + "/" + name + ".vxl";
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    char magic[8];
    f.read(magic, 8);
    if (memcmp(magic, "VOXELSAV", 8) != 0) return false;
    uint32_t ver;
    f.read((char*)&ver, sizeof(ver));
    if (ver < 1 || ver > 2) return false;
    int32_t ws, wh;
    f.read((char*)&ws, sizeof(ws));
    f.read((char*)&wh, sizeof(wh));
    if (ws != WORLD_SIZE || wh != WORLD_HEIGHT) return false;
    f.read((char*)world.blocks, sizeof(world.blocks));
    f.read((char*)world.meta, sizeof(world.meta));
    f.read((char*)world.doorOpen, sizeof(world.doorOpen));
    if (ver >= 2) {
        f.read((char*)&camera.position, sizeof(camera.position));
        f.read((char*)&camera.yaw, sizeof(camera.yaw));
        f.read((char*)&camera.pitch, sizeof(camera.pitch));
        f.read((char*)&camera.flying, sizeof(camera.flying));
    }
    return true;
}

bool deleteSave(const std::string& name) {
    std::string path = std::string(SAVE_DIR) + "/" + name + ".vxl";
    return std::filesystem::remove(path);
}

bool renameSave(const std::string& oldName, const std::string& newName) {
    std::string oldPath = std::string(SAVE_DIR) + "/" + oldName + ".vxl";
    std::string newPath = std::string(SAVE_DIR) + "/" + newName + ".vxl";
    std::filesystem::rename(oldPath, newPath);
    return true;
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
    GameState gameState = MAIN_MENU;
    std::vector<SaveInfo> saves;
    int menuSelection = 0;
    int menuScroll = 0;
    int menuMaxVisible = 8;
    bool menuConfirmDelete = false;
    std::string menuRenameInput;
    bool menuNewInput = false;
    bool menuRenameMode = false;
    std::string currentSaveName;
    int menuLoadConfirm = -1;
    BlockType selectedBlock = BLOCK_GRASS;
    BuildMode buildMode = BUILD_BLOCK;
    bool inventoryOpen = false;
    int invCursor = 0;
    float invMoveTimer = 0.0f;
    bool prevCrouching = false;
    bool firstMouse = true;
    double lastMouseX = 640.0, lastMouseY = 360.0;
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
    VkDeviceSize hudBufferSize = 0;
    uint32_t hudVertexCount = 0;

    static const int MAX_FRAMES_IN_FLIGHT = 2;

    // ==================== INIT ====================

    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
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
        if (app->inventoryOpen || app->gameState != PLAYING) return;
        if (button == GLFW_MOUSE_BUTTON_LEFT) app->breakBlock();
        if (button == GLFW_MOUSE_BUTTON_RIGHT) app->placeBlock();
    }

    static void keyCallback(GLFWwindow* w, int key, int, int action, int) {
        if (action != GLFW_PRESS) return;
        auto app = reinterpret_cast<VoxelEngine*>(glfwGetWindowUserPointer(w));
        if (key == GLFW_KEY_E) {
            if (app->gameState == PLAYING) {
                app->inventoryOpen = !app->inventoryOpen;
                glfwSetInputMode(app->window, GLFW_CURSOR,
                    app->inventoryOpen ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
                if (!app->inventoryOpen) app->firstMouse = true;
            }
            return;
        }
        if (key == GLFW_KEY_ESCAPE) {
            if (app->gameState == MAIN_MENU) glfwSetWindowShouldClose(w, true);
            else {
                if (!app->currentSaveName.empty()) saveWorld(app->world, app->camera, app->currentSaveName);
                app->gameState = MAIN_MENU;
                app->menuLoadConfirm = -1;
                glfwSetInputMode(app->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            return;
        }
        if (app->gameState != PLAYING) return;
        if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9) {
            int idx = key - GLFW_KEY_1 + 1;
            if (idx < BLOCK_COUNT) {
                app->selectedBlock = static_cast<BlockType>(idx);
                const char* modeNames[] = {"Block", "Slab", "Stair"};
                std::cout << "Selected: " << getBlockName(app->selectedBlock);
                if (app->selectedBlock != BLOCK_DOOR) std::cout << " [" << modeNames[app->buildMode] << "]";
                std::cout << "\n";
            }
        }
        if (key == GLFW_KEY_C) {
            app->buildMode = static_cast<BuildMode>((app->buildMode + 1) % 3);
            const char* modeNames[] = {"Block", "Slab", "Stair"};
            std::cout << "Build mode: " << modeNames[app->buildMode] << "\n";
        }
    }

    // ==================== VULKAN ====================

    void initVulkan() {
        initFont();
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
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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

        glm::vec3 faceNormals[6] = {
            {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
        };
        glm::ivec3 faceNeighbors[6] = {
            {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
        };
        float darkMul[6] = {0.85f, 0.85f, 1.0f, 0.6f, 0.9f, 0.9f};

        auto addBox = [&](float bx0, float by0, float bz0, float bx1, float by1, float bz1,
                           float ox, float oy, float oz, const BlockColor& color,
                           int cx, int cy, int cz, bool cullFaces = true) {
            glm::vec3 fv[6][4] = {
                {{bx1,by0,bz0},{bx1,by1,bz0},{bx1,by1,bz1},{bx1,by0,bz1}},
                {{bx0,by0,bz1},{bx0,by1,bz1},{bx0,by1,bz0},{bx0,by0,bz0}},
                {{bx0,by1,bz0},{bx0,by1,bz1},{bx1,by1,bz1},{bx1,by1,bz0}},
                {{bx0,by0,bz1},{bx0,by0,bz0},{bx1,by0,bz0},{bx1,by0,bz1}},
                {{bx0,by0,bz1},{bx0,by1,bz1},{bx1,by1,bz1},{bx1,by0,bz1}},
                {{bx1,by0,bz0},{bx1,by1,bz0},{bx0,by1,bz0},{bx0,by0,bz0}}
            };
            for (int f = 0; f < 6; f++) {
                if (cullFaces) {
                    glm::ivec3 n = glm::ivec3(cx, cy, cz) + faceNeighbors[f];
                    BlockType nb = world.get(n.x, n.y, n.z);
                    if (nb != BLOCK_AIR) {
                        uint8_t nm = world.getMeta(n.x, n.y, n.z);
                        bool fullBlock = (nm == 0) && !(nb == BLOCK_DOOR || nb == BLOCK_GLASS);
                        if (fullBlock) continue;
                    }
                }
                uint32_t base = static_cast<uint32_t>(vertices.size());
                float dm = darkMul[f];
                for (int v = 0; v < 4; v++) {
                    vertices.push_back({
                        fv[f][v] + glm::vec3(ox, oy, oz),
                        {color.r * dm, color.g * dm, color.b * dm, color.a},
                        faceNormals[f]
                    });
                }
                indices.push_back(base+0); indices.push_back(base+1); indices.push_back(base+2);
                indices.push_back(base+2); indices.push_back(base+3); indices.push_back(base+0);
            }
        };

        for (int x = 0; x < WORLD_SIZE; x++) {
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                for (int z = 0; z < WORLD_SIZE; z++) {
                    BlockType block = world.get(x, y, z);
                    if (block == BLOCK_AIR) continue;
                    uint8_t m = world.getMeta(x, y, z);
                    BlockColor color = getBlockColor(block);
                    float fx = (float)x, fy = (float)y, fz = (float)z;

                    if (m >= 10 && m <= 13) {
                        bool open = world.getDoorOpen(x, y, z);
                        float t = 0.075f;
                        int orient = m - 10;
                        if (orient == 0) {
                            if (open) addBox(0, 0, 1-2*t, 1, 1, 1, fx, fy, fz, color, x, y, z, false);
                            else addBox(0.5f-t, 0, 0, 0.5f+t, 1, 1, fx, fy, fz, color, x, y, z, false);
                        } else if (orient == 1) {
                            if (open) addBox(0, 0, 0, 1, 1, 2*t, fx, fy, fz, color, x, y, z, false);
                            else addBox(0.5f-t, 0, 0, 0.5f+t, 1, 1, fx, fy, fz, color, x, y, z, false);
                        } else if (orient == 2) {
                            if (open) addBox(1-2*t, 0, 0, 1, 1, 1, fx, fy, fz, color, x, y, z, false);
                            else addBox(0, 0, 0.5f-t, 1, 1, 0.5f+t, fx, fy, fz, color, x, y, z, false);
                        } else {
                            if (open) addBox(0, 0, 0, 2*t, 1, 1, fx, fy, fz, color, x, y, z, false);
                            else addBox(0, 0, 0.5f-t, 1, 1, 0.5f+t, fx, fy, fz, color, x, y, z, false);
                        }
                        continue;
                    }

                    if (block == BLOCK_GLASS) {
                        addBox(0.02f, 0.02f, 0.02f, 0.98f, 0.98f, 0.98f, fx, fy, fz, {0.50f, 0.75f, 0.95f}, x, y, z, false);
                        addBox(0, 0, 0, 1, 1, 1, fx, fy, fz, {0.70f, 0.90f, 1.00f}, x, y, z, true);
                        continue;
                    }

                    if (m == 0) {
                        addBox(0, 0, 0, 1, 1, 1, fx, fy, fz, color, x, y, z);
                    } else if (m == 1) {
                        addBox(0, 0, 0, 1, 0.5f, 1, fx, fy, fz, color, x, y, z);
                    } else if (m >= 2 && m <= 5) {
                        addBox(0, 0, 0, 1, 0.5f, 1, fx, fy, fz, color, x, y, z);
                        if (m == 2) addBox(0, 0.5f, 0, 1, 1, 0.5f, fx, fy, fz, color, x, y, z, false);
                        if (m == 3) addBox(0, 0.5f, 0.5f, 1, 1, 1, fx, fy, fz, color, x, y, z, false);
                        if (m == 4) addBox(0, 0.5f, 0, 0.5f, 1, 1, fx, fy, fz, color, x, y, z, false);
                        if (m == 5) addBox(0.5f, 0.5f, 0, 1, 1, 1, fx, fy, fz, color, x, y, z, false);
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
            vertices.push_back({{0,0,0},{1,0,1,1},{0,1,0}});
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
        float s = 1.005f;
        float o = -0.0025f;
        std::vector<Vertex> v = {
            {{o, o, o}, {1,1,1,1}, {0,1,0}}, {{s, o, o}, {1,1,1,1}, {0,1,0}},
            {{s, s, o}, {1,1,1,1}, {0,1,0}}, {{o, s, o}, {1,1,1,1}, {0,1,0}},
            {{o, o, s}, {1,1,1,1}, {0,1,0}}, {{s, o, s}, {1,1,1,1}, {0,1,0}},
            {{s, s, s}, {1,1,1,1}, {0,1,0}}, {{o, s, s}, {1,1,1,1}, {0,1,0}}
        };
        std::vector<uint32_t> idx = {
            0,1,1,2,2,3,3,0, 4,5,5,6,6,7,7,4,
            0,4,1,5,2,6,3,7
        };
        highlightIndexCount = static_cast<uint32_t>(idx.size());

        // Allocate enough for 2 wire boxes (stair/slab highlights)
        uint32_t maxVerts = 16;
        uint32_t maxIndices = 48;
        VkDeviceSize vs = sizeof(Vertex) * maxVerts;
        createBuffer(vs, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, highlightVB, highlightVM);
        void* d; vkMapMemory(device, highlightVM, 0, vs, 0, &d); memcpy(d, v.data(), sizeof(Vertex) * v.size()); vkUnmapMemory(device, highlightVM);

        VkDeviceSize is = sizeof(uint32_t) * maxIndices;
        createBuffer(is, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, highlightIB, highlightIM);
        vkMapMemory(device, highlightIM, 0, is, 0, &d); memcpy(d, idx.data(), sizeof(uint32_t) * idx.size()); vkUnmapMemory(device, highlightIM);
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

    int getFbWidth() { int w, h; glfwGetFramebufferSize(window, &w, &h); return w; }
    int getFbHeight() { int w, h; glfwGetFramebufferSize(window, &w, &h); return h; }

    void updateHudUniformBuffer(uint32_t imageIndex) {
        UBO2D ubo{};
        float fbw = static_cast<float>(swapChainExtent.width), fbh = static_cast<float>(swapChainExtent.height);
        ubo.proj = glm::ortho(0.0f, fbw, 0.0f, fbh, -1.0f, 1.0f);
        memcpy(hudUniformBuffersMapped[imageIndex], &ubo, sizeof(ubo));
    }

    void updateHudVertexBuffer() {
        std::vector<Vertex2D> verts;
        float fbw = static_cast<float>(swapChainExtent.width), fbh = static_cast<float>(swapChainExtent.height);

        auto quad = [&](float x, float y, float w, float h, float r, float g, float b) {
            uint32_t base = static_cast<uint32_t>(verts.size());
            verts.push_back({{x, y}, {r, g, b}});
            verts.push_back({{x + w, y}, {r, g, b}});
            verts.push_back({{x + w, y + h}, {r, g, b}});
            verts.push_back({{x, y + h}, {r, g, b}});
            verts.push_back({{x + w, y + h}, {r, g, b}});
            verts.push_back({{x, y}, {r, g, b}});
        };

        if (inventoryOpen) {
            int cols = 5, rows = 5;
            float cell = 64.0f, gap = 6.0f;
            float gridW = cols * cell + (cols - 1) * gap;
            float gridH = rows * cell + (rows - 1) * gap;
            float startX = (fbw - gridW) / 2.0f;
            float startY = (fbh - gridH) / 2.0f;
            float pp = 20.0f;
            quad(startX - pp, startY - pp - 30, gridW + pp * 2, gridH + pp * 2 + 30, 0.10f, 0.10f, 0.12f);
            std::string title = "INVENTORY  (Arrow Keys = Move, Enter = Select, E = Close)";
            float tw = title.size() * 6;
            drawText(verts, (fbw - tw) / 2, startY - pp - 24, title, 0.70f, 0.70f, 0.80f, 1);
            BlockType ct = static_cast<BlockType>(invCursor + 1);
            std::string sn = getBlockName(ct);
            float sw = sn.size() * 6 * 2;
            drawText(verts, (fbw - sw) / 2, startY + gridH + 10, sn, 0.90f, 0.90f, 1.00f, 2);
            int totalCells = cols * rows;
            for (int i = 0; i < totalCells; i++) {
                int bt = i + 1;
                if (bt >= BLOCK_COUNT) continue;
                int col = i % cols, row = i / cols;
                float x = startX + col * (cell + gap);
                float y = startY + row * (cell + gap);
                BlockColor c = getBlockColor(static_cast<BlockType>(bt));
                if (i == invCursor) {
                    float b = 3.0f;
                    quad(x - b, y - b, cell + b * 2, cell + b * 2, 1.0f, 1.0f, 1.0f);
                } else {
                    float b = 1.0f;
                    quad(x - b, y - b, cell + b * 2, cell + b * 2, 0.20f, 0.20f, 0.22f);
                }
                float pd = 2.0f;
                quad(x + pd, y + pd, cell - pd * 2, cell - pd * 2, c.r, c.g, c.b);
            }
        } else {
            float slotSize = 44.0f;
            float slotX = (fbw - slotSize) / 2.0f;
            float slotY = fbh - 70.0f;
            float b = 3.0f;
            quad(slotX - b, slotY - b, slotSize + b * 2, slotSize + b * 2, 1.0f, 1.0f, 1.0f);
            BlockColor c = getBlockColor(selectedBlock);
            float pd = 3.0f;
            float ix = slotX + pd, iy = slotY + pd;
            float iw = slotSize - pd * 2, ih = slotSize - pd * 2;
            if (selectedBlock == BLOCK_DOOR) {
                float dw = iw * 0.3f;
                quad(ix + (iw - dw) / 2, iy, dw, ih, c.r, c.g, c.b);
            } else if (buildMode == BUILD_SLAB) {
                quad(ix, iy + ih * 0.5f, iw, ih * 0.5f, c.r, c.g, c.b);
            } else if (buildMode == BUILD_STAIR) {
                quad(ix, iy + ih * 0.5f, iw, ih * 0.5f, c.r, c.g, c.b);
                quad(ix, iy, iw * 0.5f, ih * 0.5f, c.r, c.g, c.b);
            } else {
                quad(ix, iy, iw, ih, c.r, c.g, c.b);
            }
            const char* modeNames[] = {"Block", "Slab", "Stair"};
            std::string info = std::string(getBlockName(selectedBlock));
            if (selectedBlock != BLOCK_DOOR) info += std::string(" [") + modeNames[buildMode] + "]";
            float infoW = info.size() * 6 * 2;
            drawText(verts, (fbw - infoW) / 2, slotY - 20, info, 0.90f, 0.90f, 0.95f, 2);
            float cx = fbw / 2.0f, cy = fbh / 2.0f, cr = 2.0f;
            quad(cx - cr, cy - cr, cr * 2, cr * 2, 1.0f, 1.0f, 1.0f);
        }

        hudVertexCount = static_cast<uint32_t>(verts.size());
        if (hudVertexCount == 0) return;

        VkDeviceSize size = sizeof(Vertex2D) * verts.size();
        if (size > hudBufferSize) {
            vkDeviceWaitIdle(device);
            if (hudVB) {
                vkDestroyBuffer(device, hudVB, nullptr);
                vkFreeMemory(device, hudVM, nullptr);
            }
            createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         hudVB, hudVM);
            hudBufferSize = size;
        }
        void* d;
        vkMapMemory(device, hudVM, 0, size, 0, &d);
        memcpy(d, verts.data(), size);
        vkUnmapMemory(device, hudVM);
    }

    void drawText(std::vector<Vertex2D>& verts, float x, float y, const std::string& text, float r, float g, float b, int scale = 2) {
        int chW = 6 * scale, chH = 8 * scale;
        for (size_t ci = 0; ci < text.size(); ci++) {
            unsigned char c = static_cast<unsigned char>(text[ci]);
            for (int row = 0; row < 7; row++) {
                uint8_t bits = fontData[c][row];
                for (int col = 0; col < 5; col++) {
                    if (bits & (1 << col)) {
                        float px = x + ci * chW + (4 - col) * scale;
                        float py = y + row * scale;
                        uint32_t base = static_cast<uint32_t>(verts.size());
                        verts.push_back({{px, py}, {r, g, b}});
                        verts.push_back({{px + scale, py}, {r, g, b}});
                        verts.push_back({{px + scale, py + scale}, {r, g, b}});
                        verts.push_back({{px, py + scale}, {r, g, b}});
                        verts.push_back({{px + scale, py + scale}, {r, g, b}});
                        verts.push_back({{px, py}, {r, g, b}});
                    }
                }
            }
        }
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
            if (world.get(hit.x, hit.y, hit.z) == BLOCK_DOOR) {
                world.set(hit.x, hit.y, hit.z, BLOCK_AIR);
                world.setMeta(hit.x, hit.y, hit.z, 0);
                if (hit.y + 1 < WORLD_HEIGHT && world.get(hit.x, hit.y + 1, hit.z) == BLOCK_DOOR) {
                    world.set(hit.x, hit.y + 1, hit.z, BLOCK_AIR);
                    world.setMeta(hit.x, hit.y + 1, hit.z, 0);
                }
                if (hit.y - 1 >= 0 && world.get(hit.x, hit.y - 1, hit.z) == BLOCK_DOOR) {
                    world.set(hit.x, hit.y - 1, hit.z, BLOCK_AIR);
                    world.setMeta(hit.x, hit.y - 1, hit.z, 0);
                }
                meshDirty = true;
                return;
            }
            world.set(hit.x, hit.y, hit.z, BLOCK_AIR);
            world.setMeta(hit.x, hit.y, hit.z, 0);
            meshDirty = true;
        }
    }

    int getDoorFacingMeta() {
        // Door panel perpendicular to player view
        int facing = ((int)((camera.yaw + 45.0f) / 90.0f)) % 4;
        if (facing < 0) facing += 4;
        return (facing % 2 == 0) ? 12 : 10;
    }

    int getStairFacingMeta() {
        int facing = ((int)((camera.yaw + 45.0f) / 90.0f)) % 4;
        if (facing < 0) facing += 4;
        uint8_t stairTable[4] = {3, 5, 2, 4};
        return stairTable[facing];
    }

    void placeBlock() {
        glm::ivec3 hit, place;
        if (raycastBlock(camera.position, camera.front(), 8.0f, hit, place)) {
            BlockType hitBlock = world.get(hit.x, hit.y, hit.z);
            uint8_t hitMeta = world.getMeta(hit.x, hit.y, hit.z);

            if (hitBlock != BLOCK_AIR && hitMeta >= 10 && hitMeta <= 13) {
                bool open = world.getDoorOpen(hit.x, hit.y, hit.z);
                world.setDoorOpen(hit.x, hit.y, hit.z, !open);
                if (hit.y + 1 < WORLD_HEIGHT && world.get(hit.x, hit.y + 1, hit.z) == BLOCK_DOOR)
                    world.setDoorOpen(hit.x, hit.y + 1, hit.z, !open);
                if (hit.y - 1 >= 0 && world.get(hit.x, hit.y - 1, hit.z) == BLOCK_DOOR)
                    world.setDoorOpen(hit.x, hit.y - 1, hit.z, !open);
                meshDirty = true;
                return;
            }

            if (place.x >= 0 && place.x < WORLD_SIZE &&
                place.y >= 0 && place.y < WORLD_HEIGHT &&
                place.z >= 0 && place.z < WORLD_SIZE) {
                float r = 0.25f;
                float feetY = camera.position.y - camera.eyeHeight;
                float headY = camera.position.y + (camera.crouching ? 0.0f : 0.2f);
                bool insidePlayer =
                    place.x >= (int)floor(camera.position.x - r) && place.x <= (int)floor(camera.position.x + r) &&
                    place.z >= (int)floor(camera.position.z - r) && place.z <= (int)floor(camera.position.z + r) &&
                    place.y >= (int)floor(feetY) && place.y <= (int)floor(headY);
                if (!insidePlayer) {
                    if (selectedBlock == BLOCK_DOOR) {
                        world.set(place.x, place.y, place.z, BLOCK_DOOR);
                        world.setMeta(place.x, place.y, place.z, getDoorFacingMeta());
                        world.setDoorOpen(place.x, place.y, place.z, false);
                        if (place.y + 1 < WORLD_HEIGHT) {
                            world.set(place.x, place.y + 1, place.z, BLOCK_DOOR);
                            world.setMeta(place.x, place.y + 1, place.z, getDoorFacingMeta());
                            world.setDoorOpen(place.x, place.y + 1, place.z, false);
                        }
                    } else {
                        world.set(place.x, place.y, place.z, selectedBlock);
                        uint8_t m = 0;
                        if (buildMode == BUILD_SLAB) m = 1;
                        else if (buildMode == BUILD_STAIR) m = getStairFacingMeta();
                        world.setMeta(place.x, place.y, place.z, m);
                    }
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
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_ERROR_DEVICE_LOST) {
            if (result == VK_ERROR_DEVICE_LOST) recreateSwapChain();
            else recreateSwapChain();
            return;
        }

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        if (gameState == MAIN_MENU) {
            drawMenuHud();
            updateHudUniformBuffer(currentFrame);
        } else {
            updateUniformBuffer(currentFrame);
        }
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
            throw std::runtime_error("Failed to submit draw command (device lost or driver error)!");

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

        if (gameState == PLAYING) {
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
        }

        // Draw 2D HUD (menu or game HUD)
        if (gameState != PLAYING) {
            // Menu HUD already set up by drawMenuHud
        } else {
            updateHudVertexBuffer();
        }
        updateHudUniformBuffer(currentFrame);
        if (hudVertexCount > 0) {
            vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, hudPipeline);
            vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, hudPipelineLayout, 0, 1,
                                    &hudDescriptorSets[currentFrame], 0, nullptr);
            VkBuffer vbs[] = {hudVB};
            VkDeviceSize off[] = {0};
            vkCmdBindVertexBuffers(cb, 0, 1, vbs, off);
            vkCmdDraw(cb, hudVertexCount, 1, 0, 0);
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

    // ==================== MENU ====================

    void processMenuInput() {
        if (menuConfirmDelete) {
            if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
                int delIdx = (menuSelection < (int)saves.size()) ? menuSelection : (int)saves.size() - 1;
                if (delIdx >= 0 && delIdx < (int)saves.size()) {
                    deleteSave(saves[delIdx].name);
                    saves = listSaves();
                    if (menuSelection >= (int)saves.size()) menuSelection = (int)saves.size() - 1;
                }
                menuConfirmDelete = false;
            }
            if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) menuConfirmDelete = false;
            return;
        }
        if (menuRenameMode || menuNewInput) {
            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
                if (menuNewInput) {
                    std::string name = "save_" + std::to_string(saves.size() + 1);
                    saveWorld(world, camera, name);
                    saves = listSaves();
                    menuSelection = (int)saves.size() - 1;
                    menuNewInput = false;
                    std::cout << "Created save: " << name << "\n";
                }
                menuRenameMode = false;
                menuNewInput = false;
            }
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                menuRenameMode = false;
                menuNewInput = false;
            }
            return;
        }

        bool up = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS;
        bool down = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS;
        bool enter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;
        bool del = glfwGetKey(window, GLFW_KEY_DELETE) == GLFW_PRESS;
        bool r = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        bool n = glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS;

        static bool prevUp = false, prevDown = false, prevEnter = false, prevDel = false, prevR = false, prevN = false;
        bool& rUp = const_cast<bool&>(prevUp); bool& rDown = const_cast<bool&>(prevDown);
        bool& rEnt = const_cast<bool&>(prevEnter); bool& rDel = const_cast<bool&>(prevDel);
        bool& rR = const_cast<bool&>(prevR); bool& rN = const_cast<bool&>(prevN);

        if (up && !rUp && menuSelection > 0) menuSelection--;
        if (down && !rDown && menuSelection < (int)saves.size() + 3) menuSelection++;
        rUp = up; rDown = down;

        if (enter && !rEnt) {
            rEnt = enter;
            int totalItems = (int)saves.size() + 3;
            if (menuSelection < (int)saves.size()) {
                if (menuLoadConfirm == menuSelection) {
                    if (loadWorld(world, camera, saves[menuSelection].name)) {
                        currentSaveName = saves[menuSelection].name;
                        gameState = PLAYING;
                        meshDirty = true;
                        menuLoadConfirm = -1;
                        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                        std::cout << "Loaded save: " << saves[menuSelection].name << "\n";
                    }
                } else {
                    menuLoadConfirm = menuSelection;
                    std::cout << "Press Enter again to load: " << saves[menuSelection].name << "\n";
                }
            } else if (menuSelection == (int)saves.size()) {
                std::string name = "save_" + std::to_string(saves.size() + 1);
                world.generate();
                camera = Camera();
                currentSaveName = name;
                saveWorld(world, camera, name);
                saves = listSaves();
                gameState = PLAYING;
                meshDirty = true;
                menuLoadConfirm = -1;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                std::cout << "New save: " << name << "\n";
            } else if (menuSelection == (int)saves.size() + 1) {
                if (!saves.empty()) menuConfirmDelete = true;
            } else if (menuSelection == (int)saves.size() + 2) {
                if (!saves.empty()) {
                    int renameIdx = (int)saves.size() - 1;
                    std::string oldName = saves[renameIdx].name;
                    std::string newName = oldName + "_renamed";
                    renameSave(oldName, newName);
                    saves = listSaves();
                    std::cout << "Renamed " << oldName << " -> " << newName << "\n";
                }
            }
        }
        rEnt = enter;

        if (del && !rDel && !saves.empty() && menuSelection < (int)saves.size()) {
            menuConfirmDelete = true;
        }
        rDel = del;
        if (n && !rN) {
            std::string name = "save_" + std::to_string(saves.size() + 1);
            world.generate();
            camera = Camera();
            currentSaveName = name;
            saveWorld(world, camera, name);
            saves = listSaves();
            gameState = PLAYING;
            meshDirty = true;
            menuLoadConfirm = -1;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            std::cout << "New save: " << name << "\n";
            rN = n;
            return;
        }
        rN = n;

        if (menuSelection < 0) menuSelection = 0;
        int maxSel = (int)saves.size() + 3;
        if (menuSelection >= maxSel) menuSelection = maxSel - 1;
    }

    void drawMenuHud() {
        std::vector<Vertex2D> verts;
        auto quad = [&](float x, float y, float w, float h, float r, float g, float b) {
            uint32_t base = static_cast<uint32_t>(verts.size());
            verts.push_back({{x, y}, {r, g, b}});
            verts.push_back({{x + w, y}, {r, g, b}});
            verts.push_back({{x + w, y + h}, {r, g, b}});
            verts.push_back({{x, y + h}, {r, g, b}});
            verts.push_back({{x + w, y + h}, {r, g, b}});
            verts.push_back({{x, y}, {r, g, b}});
        };

        float w = (float)swapChainExtent.width, h = (float)swapChainExtent.height;

        // Background
        quad(0, 0, w, h, 0.08f, 0.08f, 0.12f);

        // Title bar
        float tw = w * 0.6f, tx = (w - tw) / 2;
        quad(tx, 40, tw, 50, 0.15f, 0.20f, 0.35f);
        quad(tx, 40, tw, 3, 0.30f, 0.45f, 0.70f);
        {
            const char* title = "VULKAN VOXEL";
            float ttl = strlen(title) * 6 * 3;
            drawText(verts, (w - ttl) / 2, 48, title, 0.85f, 0.90f, 1.00f, 3);
        }

        // Save entries
        float sy = 120;
        float sh = 44;
        float sw = w * 0.6f;
        float sx = (w - sw) / 2;

        for (int i = 0; i < (int)saves.size(); i++) {
            float ey = sy + i * (sh + 6);
            bool sel = (i == menuSelection);
            bool confirm = (i == menuLoadConfirm);
            if (sel) {
                quad(sx - 4, ey - 2, sw + 8, sh + 4, 0.30f, 0.45f, 0.70f);
            }
            if (confirm) {
                quad(sx - 2, ey, sw + 4, sh, 0.40f, 0.50f, 0.80f);
            }
            float br = (i % 2 == 0) ? 0.18f : 0.14f;
            quad(sx, ey, sw, sh, br, br, br + 0.03f);
            float dotR = 0.20f, dotG = 0.50f + (i % 3) * 0.10f, dotB = 0.30f + (i % 2) * 0.20f;
            quad(sx + 8, ey + 8, sh - 16, sh - 16, dotR, dotG, dotB);
            drawText(verts, sx + sh + 8, ey + (sh - 7 * 2) / 2, saves[i].name, 0.85f, 0.85f, 0.90f, 2);
            if (confirm) {
                std::string confirmTxt = "Press Enter again to load";
                drawText(verts, sx + sw - confirmTxt.size() * 6 - 8, ey + (sh - 7) / 2, confirmTxt, 1.0f, 1.0f, 0.5f, 1);
            }
        }

        // Bottom buttons
        float by = sy + saves.size() * (sh + 6) + 20;
        const char* labels[] = {"New Save", "Delete", "Rename"};
        const char* btnLabels[] = {"New Save", "Delete", "Rename"};
        for (int i = 0; i < 3; i++) {
            int idx = (int)saves.size() + i;
            bool sel = (idx == menuSelection);
            float bx = sx + i * ((sw / 3) + 8);
            float bw = sw / 3;
            if (sel) quad(bx - 2, by - 2, bw + 4, sh + 4, 0.30f, 0.45f, 0.70f);
            float cr = (i == 0) ? 0.20f : ((i == 1) ? 0.35f : 0.18f);
            float cg = (i == 0) ? 0.35f : ((i == 1) ? 0.15f : 0.22f);
            float cb = (i == 0) ? 0.25f : ((i == 1) ? 0.15f : 0.30f);
            quad(bx, by, bw, sh, cr, cg, cb);
            std::string label = btnLabels[i];
            float lblW = label.size() * 6 * 2;
            drawText(verts, bx + (bw - lblW) / 2, by + (sh - 7 * 2) / 2, label, 0.95f, 0.95f, 1.00f, 2);
        }

        // Instructions
        std::string instr = "W/S: Navigate  Enter: Select  N: New Save  Delete: Delete  Esc: Quit";
        float instrW = instr.size() * 6;
        drawText(verts, (w - instrW) / 2, h - 40, instr, 0.50f, 0.55f, 0.65f, 1);

        // Confirm dialog
        if (menuConfirmDelete) {
            float dw = w * 0.32f, dh = h * 0.16f;
            float dx = (w - dw) / 2, dy = (h - dh) / 2;
            quad(dx, dy, dw, dh, 0.40f, 0.10f, 0.10f);
            quad(dx + 2, dy + 2, dw - 4, dh - 4, 0.30f, 0.08f, 0.08f);
            std::string confirmTxt = "DELETE SAVE?";
            float ctw = confirmTxt.size() * 6 * 2;
            drawText(verts, (w - ctw) / 2, dy + 20, confirmTxt, 1.0f, 0.8f, 0.8f, 2);
            std::string ynTxt = "Y: Yes  N: No";
            float ynw = ynTxt.size() * 6;
            drawText(verts, (w - ynw) / 2, dy + dh - 30, ynTxt, 0.8f, 0.8f, 0.9f, 1);
        }

        hudVertexCount = static_cast<uint32_t>(verts.size());
        if (hudVertexCount == 0) return;

        VkDeviceSize size = sizeof(Vertex2D) * verts.size();
        if (size > hudBufferSize) {
            vkDeviceWaitIdle(device);
            if (hudVB) {
                vkDestroyBuffer(device, hudVB, nullptr);
                vkFreeMemory(device, hudVM, nullptr);
            }
            createBuffer(size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         hudVB, hudVM);
            hudBufferSize = size;
        }
        void* d;
        vkMapMemory(device, hudVM, 0, size, 0, &d);
        memcpy(d, verts.data(), size);
        vkUnmapMemory(device, hudVM);
    }

    // ==================== MAIN LOOP ====================

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            if (gameState == MAIN_MENU) {
                saves = listSaves();
                processMenuInput();
                drawFrame();
                glfwPollEvents();
                continue;
            }

            processInput();

            // Raycast for highlight
            glm::ivec3 hit, place;
            if (raycastBlock(camera.position, camera.front(), 8.0f, hit, place)) {
                highlightedBlock = hit;
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
        float s = 0.005f;
        float o = -0.0025f;
        std::vector<Vertex> v;
        std::vector<uint32_t> idx;

        BlockType bt = world.get(pos.x, pos.y, pos.z);
        uint8_t m = world.getMeta(pos.x, pos.y, pos.z);

        auto addWireBox = [&](float bx0, float by0, float bz0, float bx1, float by1, float bz1) {
            uint32_t base = static_cast<uint32_t>(v.size());
            v.push_back({{pos.x+bx0+o, pos.y+by0+o, pos.z+bz0+o}, {1,1,1,1}, {0,1,0}});
            v.push_back({{pos.x+bx1+s, pos.y+by0+o, pos.z+bz0+o}, {1,1,1,1}, {0,1,0}});
            v.push_back({{pos.x+bx1+s, pos.y+by1+s, pos.z+bz0+o}, {1,1,1,1}, {0,1,0}});
            v.push_back({{pos.x+bx0+o, pos.y+by1+s, pos.z+bz0+o}, {1,1,1,1}, {0,1,0}});
            v.push_back({{pos.x+bx0+o, pos.y+by0+o, pos.z+bz1+s}, {1,1,1,1}, {0,1,0}});
            v.push_back({{pos.x+bx1+s, pos.y+by0+o, pos.z+bz1+s}, {1,1,1,1}, {0,1,0}});
            v.push_back({{pos.x+bx1+s, pos.y+by1+s, pos.z+bz1+s}, {1,1,1,1}, {0,1,0}});
            v.push_back({{pos.x+bx0+o, pos.y+by1+s, pos.z+bz1+s}, {1,1,1,1}, {0,1,0}});
            uint32_t b = base;
            idx.push_back(b+0); idx.push_back(b+1); idx.push_back(b+1); idx.push_back(b+2);
            idx.push_back(b+2); idx.push_back(b+3); idx.push_back(b+3); idx.push_back(b+0);
            idx.push_back(b+4); idx.push_back(b+5); idx.push_back(b+5); idx.push_back(b+6);
            idx.push_back(b+6); idx.push_back(b+7); idx.push_back(b+7); idx.push_back(b+4);
            idx.push_back(b+0); idx.push_back(b+4); idx.push_back(b+1); idx.push_back(b+5);
            idx.push_back(b+2); idx.push_back(b+6); idx.push_back(b+3); idx.push_back(b+7);
        };

        if (bt == BLOCK_DOOR && m >= 10 && m <= 13) {
            bool open = world.getDoorOpen(pos.x, pos.y, pos.z);
            float t = 0.075f;
            int orient = m - 10;
            if (orient == 0) {
                if (open) addWireBox(0, 0, 1-2*t, 1, 1, 1);
                else addWireBox(0.5f-t, 0, 0, 0.5f+t, 1, 1);
            } else if (orient == 1) {
                if (open) addWireBox(0, 0, 0, 1, 1, 2*t);
                else addWireBox(0.5f-t, 0, 0, 0.5f+t, 1, 1);
            } else if (orient == 2) {
                if (open) addWireBox(1-2*t, 0, 0, 1, 1, 1);
                else addWireBox(0, 0, 0.5f-t, 1, 1, 0.5f+t);
            } else {
                if (open) addWireBox(0, 0, 0, 2*t, 1, 1);
                else addWireBox(0, 0, 0.5f-t, 1, 1, 0.5f+t);
            }
        } else if (m == 1) {
            addWireBox(0, 0, 0, 1, 0.5f, 1);
        } else if (m >= 2 && m <= 5) {
            addWireBox(0, 0, 0, 1, 0.5f, 1);
            if (m == 2) addWireBox(0, 0.5f, 0, 1, 1, 0.5f);
            if (m == 3) addWireBox(0, 0.5f, 0.5f, 1, 1, 1);
            if (m == 4) addWireBox(0, 0.5f, 0, 0.5f, 1, 1);
            if (m == 5) addWireBox(0.5f, 0.5f, 0, 1, 1, 1);
        } else {
            addWireBox(0, 0, 0, 1, 1, 1);
        }

        VkDeviceSize vs = sizeof(Vertex) * v.size();
        void* d;
        vkMapMemory(device, highlightVM, 0, vs, 0, &d);
        memcpy(d, v.data(), vs);
        vkUnmapMemory(device, highlightVM);

        VkDeviceSize is = sizeof(uint32_t) * idx.size();
        highlightIndexCount = static_cast<uint32_t>(idx.size());
        vkMapMemory(device, highlightIM, 0, is, 0, &d);
        memcpy(d, idx.data(), is);
        vkUnmapMemory(device, highlightIM);
    }

    void processInput() {
        if (inventoryOpen) {
            invMoveTimer -= deltaTime;
            if (invMoveTimer <= 0.0f) {
                int dx = 0, dy = 0;
                if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) dx = 1;
                if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) dx = -1;
                if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) dy = 1;
                if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) dy = -1;
                if (dx != 0 || dy != 0) {
                    int col = invCursor % 5;
                    int row = invCursor / 5;
                    col = std::clamp(col + dx, 0, 4);
                    row = std::clamp(row + dy, 0, 4);
                    invCursor = row * 5 + col;
                    invMoveTimer = 0.15f;
                }
            }
            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
                selectedBlock = static_cast<BlockType>(invCursor + 1);
                inventoryOpen = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
            }
            return;
        }

        if (camera.position.y < -10.0f) {
            camera.position = {WORLD_SIZE / 2.0f, 7.0f, WORLD_SIZE / 2.0f + 15.0f};
            camera.velocityY = 0;
            camera.flying = false;
            camera.onGround = false;
        }

        bool spaceDown = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool shiftDown = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

        if (!camera.flying) {
            camera.crouching = shiftDown;
        } else {
            camera.crouching = false;
        }

        float targetEyeHeight = camera.crouching ? 1.2f : 1.6f;
        float eyeDelta = camera.eyeHeight - targetEyeHeight;
        camera.eyeHeight = targetEyeHeight;
        camera.position.y -= eyeDelta;

        glm::vec3 moveDir(0);
        float v = camera.speed * deltaTime;
        if (camera.crouching) v *= 0.4f;
        glm::vec3 fwd = camera.flying ? camera.front() : camera.flatFront();
        glm::vec3 rt = camera.right();

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += fwd;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= fwd;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= rt;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += rt;

        if (camera.flying) {
            if (glm::length(moveDir) > 0.001f) {
                moveDir = glm::normalize(moveDir) * v;
                camera.position.x += moveDir.x;
                if (isPlayerColliding(camera, world)) camera.position.x -= moveDir.x;
                camera.position.z += moveDir.z;
                if (isPlayerColliding(camera, world)) camera.position.z -= moveDir.z;
            }

            if (spaceDown) camera.position.y += camera.flySpeed * deltaTime;
            if (shiftDown) camera.position.y -= camera.flySpeed * deltaTime;

            int bx = static_cast<int>(floor(camera.position.x));
            int bz = static_cast<int>(floor(camera.position.z));
            for (int y = static_cast<int>(floor(camera.position.y)); y >= 0; y--) {
                if (world.get(bx, y, bz) != BLOCK_AIR) {
                    if (camera.position.y - camera.eyeHeight - (y + 1) < 1.5f && shiftDown) {
                        camera.flying = false;
                        camera.velocityY = 0;
                    }
                    break;
                }
            }

            if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !camera.fWasPressed) {
                camera.flying = false;
                camera.velocityY = 0;
            }
            camera.fWasPressed = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;
        } else {
            if (glm::length(moveDir) > 0.001f) {
                moveDir = glm::normalize(moveDir) * v;
                camera.position.x += moveDir.x;
                if (isPlayerColliding(camera, world)) camera.position.x -= moveDir.x;
                camera.position.z += moveDir.z;
                if (isPlayerColliding(camera, world)) camera.position.z -= moveDir.z;
            }

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

int main(int argc, char* argv[]) {
    try {
        namespace fs = std::filesystem;
        fs::current_path(fs::absolute(argv[0]).parent_path());
        VoxelEngine engine;
        engine.run();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        std::ofstream log("error.log");
        if (log.is_open()) log << "FATAL: " << e.what() << std::endl;
#ifdef _WIN32
        MessageBoxA(NULL, e.what(), "VulkanVoxel Error", MB_OK | MB_ICONERROR);
#endif
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
