// Function declarations

void Update(float deltaX, float deltaY, float deltaZ);
void Render(int offsetX, int offsetY);
void SetModel(const float* vertices, int vertexCount, const unsigned char* faces, int faceCount, int verticesPerFace, bool useProgmem = false);
