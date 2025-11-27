#include "model3d.h"
#include <cmath>
#include <TFT_eSPI.h>
#include "rainbow.h"

extern TFT_eSPI tft;
extern TFT_eSprite sprite;
//extern uint16_t modelColors[6];

static float colCycle = 0;
static float colCycleDelta = 0.01;
static float scale = 80.0f;


static signed char CubeVertices[8][3] = {
    {-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1},
    {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},  {-1, 1, 1}
};

static unsigned char CubeEdges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {4, 5}, {5, 6}, {6, 7}, {7, 4},
    {0, 4}, {1, 5}, {2, 6}, {3, 7}
};

static unsigned char CubeFaces[6][4] = {
    {0, 3, 2, 1}, // Back face (z = -1), CCW from outside
    {4, 5, 6, 7}, // Front face (z = +1), CCW from outside
    {0, 1, 5, 4}, // Bottom face (y = -1), CCW from outside
    {3, 7, 6, 2}, // Top face (y = +1), CCW from outside
    {0, 4, 7, 3}, // Left face (x = -1), CCW from outside
    {1, 2, 6, 5}  // Right face (x = +1), CCW from outside
};

// Model configuration (set via SetModel function)
static const float* currentModelVertices = nullptr;
static const unsigned char* currentModelFaces = nullptr;
static int currentVertexCount = 8;
static int currentFaceCount = 6;
static int currentVerticesPerFace = 4;
static bool currentUseProgmem = false;

// Buffer to store transformed vertices (world coordinates)
// Max size for larger models like dodecahedron (80 vertices)
static float transformedVertices[80][3];
// Buffer to store projected 2D screen coordinates
static int screenVertices[80][2];

static float xRotation = 0.0f;
static float yRotation = 0.0f;
static float zRotation = 0.0f;

// Cached sin/cos values (computed once per frame)
static float cosX, sinX, cosY, sinY, cosZ, sinZ;

void Update(float deltaX, float deltaY, float deltaZ) {
    xRotation += deltaX;
    yRotation += deltaY;
    zRotation += deltaZ;
    
    // Precompute sin/cos for the entire frame
    float xRad = xRotation * PI / 180.0f;
    float yRad = yRotation * PI / 180.0f;
    float zRad = zRotation * PI / 180.0f;
    
    cosX = cosf(xRad); sinX = sinf(xRad);
    cosY = cosf(yRad); sinY = sinf(yRad);
    cosZ = cosf(zRad); sinZ = sinf(zRad);
}

// Configure which 3D model to render
void SetModel(const float* vertices, int vertexCount, const unsigned char* faces, int faceCount, int verticesPerFace, bool useProgmem) {
    currentModelVertices = vertices;
    currentModelFaces = faces;
    currentVertexCount = vertexCount;
    currentFaceCount = faceCount;
    currentVerticesPerFace = verticesPerFace;
    currentUseProgmem = useProgmem;
}

// Transform a single vertex through rotation matrices (uses cached sin/cos from Update)
void TransformVertex(float x, float y, float z, float* outX, float* outY, float* outZ) {
    // Rotate around X axis
    float yTemp = y * cosX - z * sinX;
    float zTemp = y * sinX + z * cosX;
    y = yTemp; z = zTemp;
    
    // Rotate around Y axis
    float xTemp = x * cosY + z * sinY;
    zTemp = -x * sinY + z * cosY;
    x = xTemp; z = zTemp;
    
    // Rotate around Z axis
    xTemp = x * cosZ - y * sinZ;
    yTemp = x * sinZ + y * cosZ;
    x = xTemp; y = yTemp;
    
    *outX = x;
    *outY = y;
    *outZ = z;
}

void Render(int offsetX, int offsetY) {
    // Clear the sprite
    sprite.fillSprite(TFT_BLACK);

    // Use default cube if no model is set
    const float* modelVertices = currentModelVertices ? currentModelVertices : (const float*)CubeVertices;
    const unsigned char* modelFaces = currentModelFaces ? currentModelFaces : (const unsigned char*)CubeFaces;
    int vertexCount = currentVertexCount;
    int faceCount = currentFaceCount;
    int verticesPerFace = currentVerticesPerFace;
    bool useProgmem = currentUseProgmem;
    
    // Step 1: Transform all vertices once
    for (int i = 0; i < vertexCount; i++) {
        float x, y, z;
        if (useProgmem) {
            int idx = i * 3;
            x = pgm_read_float(&modelVertices[idx]);
            y = pgm_read_float(&modelVertices[idx + 1]);
            z = pgm_read_float(&modelVertices[idx + 2]);
        } else {
            int idx = i * 3;
            x = modelVertices[idx];
            y = modelVertices[idx + 1];
            z = modelVertices[idx + 2];
        }
        
        TransformVertex(x, y, z, 
                       &transformedVertices[i][0], 
                       &transformedVertices[i][1], 
                       &transformedVertices[i][2]);
        
        // Project to 2D screen coordinates
        screenVertices[i][0] = (int)(transformedVertices[i][0] * scale) + offsetX;
        screenVertices[i][1] = (int)(transformedVertices[i][1] * scale) + offsetY;
    }
    
    // Step 2: Draw faces (filled polygons) with backface culling
    for (int i = 0; i < faceCount; i++) {
        // Get the vertex indices for this face
        int faceIdx = i * verticesPerFace;
        int v0, v1, v2, v3;
        if (useProgmem) {
            v0 = pgm_read_byte(&modelFaces[faceIdx]);
            v1 = pgm_read_byte(&modelFaces[faceIdx + 1]);
            v2 = pgm_read_byte(&modelFaces[faceIdx + 2]);
            v3 = (verticesPerFace == 4) ? pgm_read_byte(&modelFaces[faceIdx + 3]) : v2;
        } else {
            v0 = modelFaces[faceIdx];
            v1 = modelFaces[faceIdx + 1];
            v2 = modelFaces[faceIdx + 2];
            v3 = (verticesPerFace == 4) ? modelFaces[faceIdx + 3] : v2;
        }
        
        // Backface culling using 2D screen space winding order
        // Calculate the signed area of the triangle (v0, v1, v2) in screen space
        // Positive area = counter-clockwise winding = front-facing
        // Negative area = clockwise winding = back-facing
        int dx1 = screenVertices[v1][0] - screenVertices[v0][0];
        int dy1 = screenVertices[v1][1] - screenVertices[v0][1];
        int dx2 = screenVertices[v2][0] - screenVertices[v0][0];
        int dy2 = screenVertices[v2][1] - screenVertices[v0][1];
        
        // Cross product in 2D (z component only)
        int crossZ = dx1 * dy2 - dy1 * dx2;
        
        // If crossZ < 0, the triangle is clockwise (back-facing), so skip it
        if (crossZ < 0) {
            continue;
        }
      
        // Calculate fancy colors using Z cross vector value
        int colorIdx = ((int)(abs(crossZ) * 0.0695f) + (int)colCycle) % 180;
        const unsigned char* colorPtr = rbow[colorIdx];
        
        // Read RGB values from PROGMEM and convert to RGB565 in one step
        uint16_t color = (pgm_read_byte(&colorPtr[0]) & 0xF8) << 8 |
                         (pgm_read_byte(&colorPtr[1]) & 0xFC) << 3 |
                         (pgm_read_byte(&colorPtr[2]) >> 3);
        
        // Update color cycle per face for flowing rainbow effect
        colCycle += colCycleDelta;
        if (colCycle > 134 || colCycle < 0)
            colCycleDelta = -colCycleDelta;

        // Different colors for each face for better visibility
        // Cycle through colors for models with many faces
        //uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_MAGENTA, TFT_CYAN};
        //uint16_t color = colors[i % 6];
        
        // Draw filled quadrilateral (split into two triangles)
        sprite.fillTriangle(
            screenVertices[v0][0], screenVertices[v0][1],
            screenVertices[v1][0], screenVertices[v1][1],
            screenVertices[v2][0], screenVertices[v2][1],
            color);
        // Only draw second triangle if we have a quad (4 vertices)
        if (verticesPerFace == 4) {
            sprite.fillTriangle(
                screenVertices[v0][0], screenVertices[v0][1],
                screenVertices[v2][0], screenVertices[v2][1],
                screenVertices[v3][0], screenVertices[v3][1],
                color);
        }
    }
    
    // Step 3: Draw edges (wireframe on top of faces)
    /*
    for (int i = 0; i < edgeCount; i++) {
        int v1 = modelEdges[i][0];
        int v2 = modelEdges[i][1];
        
        sprite.drawLine(
            screenVertices[v1][0], screenVertices[v1][1],
            screenVertices[v2][0], screenVertices[v2][1],
            TFT_WHITE);
    }
    */

    // Push sprite to display
    sprite.pushRotated(90);
}


/*
float GetFaceCrossVectorValue(int faceIndex) {
    // Get the vertex indices for this face
    int v0, v1, v2;
    if (currentUseProgmem) {
        v0 = pgm_read_byte(&currentModelFaces[faceIndex * currentVerticesPerFace + 0]);
        v1 = pgm_read_byte(&currentModelFaces[faceIndex * currentVerticesPerFace + 1]);
        v2 = pgm_read_byte(&currentModelFaces[faceIndex * currentVerticesPerFace + 2]);
    } else {
        v0 = currentModelFaces[faceIndex * currentVerticesPerFace + 0];
        v1 = currentModelFaces[faceIndex * currentVerticesPerFace + 1];
        v2 = currentModelFaces[faceIndex * currentVerticesPerFace + 2];
    }
    
    // Calculate vectors A and B
    float Ax = transformedVertices[v1][0] - transformedVertices[v0][0];
    float Ay = transformedVertices[v1][1] - transformedVertices[v0][1];
    float Az = transformedVertices[v1][2] - transformedVertices[v0][2];
    
    float Bx = transformedVertices[v2][0] - transformedVertices[v0][0];
    float By = transformedVertices[v2][1] - transformedVertices[v0][1];
    float Bz = transformedVertices[v2][2] - transformedVertices[v0][2];
    
    // Cross product A x B
    float crossX = Ay * Bz - Az * By;
    float crossY = Az * Bx - Ax * Bz;
    float crossZ = Ax * By - Ay * Bx;
    
    return crossZ; // Return Z component for backface culling
}
    */