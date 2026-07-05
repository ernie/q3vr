#include "vr_gl_virtual_screen.h"

#include "../vrcommon/common/xr_linear.h"
#include "../vrcommon/vr_virtual_screen.h"

extern cvar_t *vr_virtualScreenShape;

typedef enum
{
	CURVED = 0,
	FLAT = 1,
} virtualScreenShape_t;

// Flat VirtualScreen
float quadVertices[] =
{
	// position            uv
	 0.5f,  0.5f, 0.0f,    1.0f, 1.0f,  // top right
	 0.5f, -0.5f, 0.0f,    1.0f, 0.0f,  // bottom right
	-0.5f, -0.5f, 0.0f,    0.0f, 0.0f,  // bottom left
	-0.5f,  0.5f, 0.0f,    0.0f, 1.0f,  // top left 
};
unsigned int quadIndices[] =
{
	0, 1, 3,   // first triangle
	1, 2, 3,   // second triangle
};

const char* vsVertexShaderSource =
	"#version 430 core\n"
	"\n"
	"#define NUM_VIEWS 2\n"
	"#extension GL_OVR_multiview2 : enable\n"
	"layout(num_views=NUM_VIEWS) in;\n"
	"\n"
	"layout (location = 0) in vec3 aPos;\n"
	"layout (location = 1) in vec2 aTexCoord;\n"
	"\n"
	"out vec2 TexCoord;\n"
	"\n"
	"layout (location = 2) uniform mat4 model;\n"
	"layout (location = 3) uniform mat4 view[2];\n"
	"layout (location = 5) uniform mat4 proj[2];\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = (proj[gl_ViewID_OVR] * (view[gl_ViewID_OVR] * (model * vec4(aPos.x, aPos.y, aPos.z, 1.0))));\n"
	"    TexCoord = aTexCoord;\n"
	"}\n";
const char* vsFragmentShaderSource =
	"#version 330 core\n"
	"out vec4 FragColor;\n"
	"\n"
	"in vec2 TexCoord;\n"
	"\n"
	"uniform sampler2D renderedTexture;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    FragColor = texture(renderedTexture, TexCoord);\n"
	"}\n";

const char* floorVertexShaderSource =
	"#version 430 core\n"
	"\n"
	"#define NUM_VIEWS 2\n"
	"#extension GL_OVR_multiview2 : enable\n"
	"layout(num_views=NUM_VIEWS) in;\n"
	"\n"
	"layout (location = 0) in vec3 aPos;\n"
	"layout (location = 1) in vec2 aTexCoord;\n"
	"\n"
	"out vec2 Pos;\n"
	"\n"
	"layout (location = 2) uniform mat4 model;\n"
	"layout (location = 3) uniform mat4 view[2];\n"
	"layout (location = 5) uniform mat4 proj[2];\n"
	"\n"
	"void main()\n"
	"{\n"
	"    gl_Position = (proj[gl_ViewID_OVR] * (view[gl_ViewID_OVR] * (model * vec4(aPos.xzy, 1.0))));\n"
	"    Pos = aTexCoord - vec2(0.5);"
	"}\n";
const char* floorFragmentShaderSource =
	"#version 330 core\n"
	"out vec4 FragColor;\n"
	"\n"
	"in vec2 Pos;\n"
	"\n"
	"uniform vec3 camera;\n"
	"\n"
	"void main()\n"
	"{\n"
	"    vec2 P = Pos * 30.0;\n"
	"\n"
	"    vec2 f = fract(P);\n"
	"    vec2 d = min(f, 1.0 - f);\n"
	"    vec2 w = max(fwidth(P) * 1.5, vec2(1e-6));\n"
	"    vec2 a = 1.0 - smoothstep(vec2(0.0), w, d);\n"
	"    float lineAlpha = max(a.x, a.y);\n"
	"    float dotRadius = 0.02;\n"
	"    float dist = length(d);\n"
	"    float dotAlpha =  1.0 - smoothstep(0.0, dotRadius, dist);\n"
	"\n"
	"    vec4 background = vec4(0.0);\n"
	"    vec4 gridGrey = vec4(vec3(0.075), 1.0);\n"
	"    vec4 dotWhite = vec4(vec3(0.5), 1.0);\n"
	"\n"
	"    vec4 col1 = mix(background, gridGrey, lineAlpha);\n"
	"    vec4 col2 = mix(background, dotWhite, dotAlpha);\n"
	"\n"
	"    float distToCamera = length(vec3(P.x, 0.0, P.y) - camera);\n"
	"\n"
	"    FragColor = max(col1, col2);\n"
	"\n"
	"    FragColor.gb *= 1.0 - (distToCamera / 5);\n"
	"    FragColor.a *= 1.0 - (distToCamera / 15);\n"
	"}\n";

int cylinderVertexCount = 0;
int cylinderIndexCount = 0;
int quadVertexCount = sizeof(quadVertices)/sizeof(quadVertices[0]);
int quadIndexCount = sizeof(quadIndices)/sizeof(quadIndices[0]);

unsigned int cylinderVBO = 0;
unsigned int cylinderVAO = 0;
unsigned int cylinderEBO = 0;
unsigned int quadVBO = 0;
unsigned int quadVAO = 0;
unsigned int quadEBO = 0;

unsigned int vsShaderProgram = 0;
unsigned int floorShaderProgram = 0;

// Cached uniform locations (using explicit layout locations from shaders)
// Shader uses: layout (location = 2) uniform mat4 model;
//              layout (location = 3) uniform mat4 view[2];
//              layout (location = 5) uniform mat4 proj;
#define UNIFORM_LOC_MODEL 2
#define UNIFORM_LOC_VIEW  3
#define UNIFORM_LOC_PROJ  5

// Floor shader has camera uniform without explicit location, cache it
static GLint floorUniformCamera = -1;

unsigned int _VR_CreateAndCompileShader(GLenum shaderType, const GLchar** source)
{
	const unsigned int shader = qglCreateShader(shaderType);
	CHECK(shader != 0, "Failed to create shader");
	qglShaderSource(shader, 1, source, NULL);
	qglCompileShader(shader);

	int success;
	char infoLog[512];
	qglGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		qglGetShaderInfoLog(shader, 512, NULL, infoLog);
		fprintf(stderr, "Failed to compile shader:\n%s\n", infoLog);
		exit(1);
	}
	return shader;
}

unsigned int _VR_CreateAndLinkShaderProgram(unsigned int vertexShader, unsigned int fragmentShader)
{
	const unsigned int program = qglCreateProgram();
	qglAttachShader(program, vertexShader);
	qglAttachShader(program, fragmentShader);
	qglLinkProgram(program);

	int success;
	char infoLog[512];
	qglGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success)
	{
		qglGetProgramInfoLog(program, 512, NULL, infoLog);
		fprintf(stderr, "Failed to link program:\n%s\n", infoLog);
		exit(1);
	}
	return program;
}

void generateCylinderSectionSimple(
	float radius, float height, float arcAngle, int segments,
	float** outVertices, int* outVertexCount,
	unsigned int** outIndices, int* outIndexCount)
{
	int vertsPerRow = segments + 1;
	int totalVerts = vertsPerRow * 2;
	int floatsPerVert = 5;

	*outVertexCount = totalVerts;
	*outVertices = (float*)malloc(totalVerts * floatsPerVert * sizeof(float));

	int totalIndices = segments * 6;
	*outIndexCount = totalIndices;
	*outIndices = (unsigned int*)malloc(totalIndices * sizeof(unsigned int));

	float* vptr = *outVertices;
	unsigned int* iptr = *outIndices;

	float startAngle = -arcAngle * 0.5f;
	float dTheta = arcAngle / (float)segments;

	for (int i = 0; i <= segments; i++)
	{
		float theta = startAngle + i * dTheta;

		// Generate cylinder curving toward -Z (away from origin in local space)
		// After model transform (placed in front of user, rotated 180° to face them),
		// this creates a screen that wraps around the user (center further, edges closer)
		float x = radius * sinf(theta);
		float z = -radius * cosf(theta);  // Negative Z - center is furthest from origin
		float u = (float)i / (float)segments;

		*vptr++ = x; *vptr++ =  height * 0.5f; *vptr++ = z;
		*vptr++ = u; *vptr++ = 1.0f;

		*vptr++ = x; *vptr++ = -height * 0.5f; *vptr++ = z;
		*vptr++ = u; *vptr++ = 0.0f;
	}

	for (int i = 0; i < segments; i++)
	{
		int top0 = i * 2;
		int bot0 = top0 + 1;
		int top1 = top0 + 2;
		int bot1 = bot0 + 2;

		*iptr++ = top0; *iptr++ = top1; *iptr++ = bot0;
		*iptr++ = bot0; *iptr++ = top1; *iptr++ = bot1;
	}
}

unsigned int _VR_CreateVaoAndProgram(
	unsigned int* VAO, 
	unsigned int* VBO, 
	unsigned int* EBO,
	float* vertices,
	unsigned int vertexCount,
	unsigned int* indices,
	unsigned int indexCount,
	const char* vertexShaderSrc,
	const char* framgnetShaderSrc)
{
	// VAO
	qglGenVertexArrays(1, VAO);
	CHECK(VAO != 0, "Failed to create VAO");
	qglBindVertexArray(*VAO);
	// VBO
	qglGenBuffers(1, VBO);
	qglBindBuffer(GL_ARRAY_BUFFER, *VBO);
	qglBufferData(GL_ARRAY_BUFFER, vertexCount * 5 * sizeof(float), vertices, GL_STATIC_DRAW);
	qglVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	qglVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	qglEnableVertexAttribArray(0);
	qglEnableVertexAttribArray(1);
	// EBO
	qglGenBuffers(1, EBO);
	CHECK(EBO != 0, "Failed to create EBO");
	qglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *EBO);
	qglBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);
	// Shaders
	unsigned int vertexShader = _VR_CreateAndCompileShader(GL_VERTEX_SHADER, &vertexShaderSrc);
	unsigned int fragmentShader = _VR_CreateAndCompileShader(GL_FRAGMENT_SHADER, &framgnetShaderSrc);
	unsigned int shaderProgram = _VR_CreateAndLinkShaderProgram(vertexShader, fragmentShader);
	qglDeleteShader(vertexShader);
	qglDeleteShader(fragmentShader);

	return shaderProgram;
}

void VR_VirtualScreen_Init(void)
{
	CHECK(cylinderVBO == 0, "Can be called only once");
	CHECK(cylinderVBO == 0, "Can be called only once");

	// Virtual screen
	float* cylinderVertices;
	unsigned int* cylinderIndices;
	generateCylinderSectionSimple(2.5, 3.5f, M_PI / 2.0f, 64, &cylinderVertices, &cylinderVertexCount, &cylinderIndices, &cylinderIndexCount);

	vsShaderProgram = _VR_CreateVaoAndProgram(
		&cylinderVAO, &cylinderVBO, &cylinderEBO,
		cylinderVertices, cylinderVertexCount,
		cylinderIndices, cylinderIndexCount,
		vsVertexShaderSource, vsFragmentShaderSource
	);

	free(cylinderVertices);
	free(cylinderIndices);

	// Floor
	floorShaderProgram = _VR_CreateVaoAndProgram(
		&quadVAO, &quadVBO, &quadEBO,
		quadVertices, quadVertexCount,
		quadIndices, quadIndexCount,
		floorVertexShaderSource, floorFragmentShaderSource
	);

	// Cache the camera uniform location (only one without explicit layout location)
	floorUniformCamera = qglGetUniformLocation(floorShaderProgram, "camera");

	// Unbind VAO to ensure nobody modifies it
	qglBindVertexArray(0);
}

void VR_VirtualScreen_Destroy(void)
{
	qglDeleteProgram(vsShaderProgram);
	qglDeleteProgram(floorShaderProgram);
	vsShaderProgram = 0;
	floorShaderProgram = 0;

	qglDeleteBuffers(1, &cylinderEBO);
	qglDeleteBuffers(1, &cylinderVBO);
	qglDeleteVertexArrays(1, &cylinderVAO);
	cylinderEBO = 0;
	cylinderVBO = 0;
	cylinderVAO = 0;

	qglDeleteBuffers(1, &quadEBO);
	qglDeleteBuffers(1, &quadVBO);
	qglDeleteVertexArrays(1, &quadVAO);
	quadEBO = 0;
	quadVBO = 0;
	quadVAO = 0;
}

void VR_VirtualScreen_Draw(XrView* views, uint32_t viewCount, GLuint virtualScreenImage)
{
	GLuint previousVAO, previousProgram, previousTexture;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&previousVAO);
	glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&previousProgram);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&previousTexture);

	const GLboolean prevBlend = glIsEnabled(GL_BLEND);
	const GLboolean prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
	GLboolean depthMask;
	qglGetBooleanv(GL_DEPTH_WRITEMASK, &depthMask);

	// Depth test must be disabled: the Q3 renderer leaves it enabled after
	// scene rendering, and depth buffer values cause virtual screen fragments
	// to be rejected even after clearing.
	qglDisable(GL_DEPTH_TEST);

	if (!prevBlend)
	{
		qglEnable(GL_BLEND);
		qglBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	if (depthMask == GL_TRUE)
	{
		qglDepthMask(GL_FALSE);
	}

	// Get per-eye poses
	XrPosef* left = &views[0].pose;
	XrPosef* right = &views[viewCount > 1 ? viewCount - 1 : 0].pose;

	// Compute centered head pose (midpoint between eyes) for model matrix positioning
	// This ensures the virtual screen appears centered, not offset to one eye
	XrPosef centeredHead;
	VR_VirtualScreen_ComputeCenteredHeadPose(&centeredHead, views, viewCount);

	// Compute Model, View(s) and per-eye Projection matrices
	XrMatrix4x4f model, view[2], projection[2];

	XrMatrix4x4f_CreateIdentity(&model);
	XrMatrix4x4f_CreateIdentity(&view[0]);
	XrMatrix4x4f_CreateIdentity(&view[1]);
	XrMatrix4x4f_CreateIdentity(&projection[0]);
	XrMatrix4x4f_CreateIdentity(&projection[1]);

	VR_VirtualScreen_GetViewMatrix(&view[0], &left->position, &left->orientation);
	VR_VirtualScreen_GetViewMatrix(&view[1], &right->position, &right->orientation);

	// Create per-eye projection matrices for proper stereo
	XrMatrix4x4f_CreateProjectionFov(&projection[0], GRAPHICS_OPENGL, views[0].fov, 0.01f, 100.0f);
	if (viewCount > 1) {
		XrMatrix4x4f_CreateProjectionFov(&projection[1], GRAPHICS_OPENGL, views[viewCount - 1].fov, 0.01f, 100.0f);
	} else {
		projection[1] = projection[0];
	}

	// Floor
	{
		VR_VirtualScreen_GetFloorModelMatrix(&model);

		qglUseProgram(floorShaderProgram);
		qglBindVertexArray(quadVAO);

		// Use explicit layout locations from shader (no runtime lookup needed)
		qglUniformMatrix4fv(UNIFORM_LOC_MODEL, 1, GL_FALSE, (float*)model.m);
		qglUniformMatrix4fv(UNIFORM_LOC_VIEW + 0, 1, GL_FALSE, (float*)view[0].m);
		qglUniformMatrix4fv(UNIFORM_LOC_VIEW + 1, 1, GL_FALSE, (float*)view[1].m);
		qglUniformMatrix4fv(UNIFORM_LOC_PROJ + 0, 1, GL_FALSE, (float*)projection[0].m);
		qglUniformMatrix4fv(UNIFORM_LOC_PROJ + 1, 1, GL_FALSE, (float*)projection[1].m);
		qglUniform3f(floorUniformCamera, left->position.x, left->position.y, left->position.z);

		glDrawElements(GL_TRIANGLES, quadIndexCount, GL_UNSIGNED_INT, 0);
	}

	// Virtual Screen
	{
		unsigned int VAO = (vr_virtualScreenShape->integer == CURVED) ? cylinderVAO : quadVAO;
		unsigned int indexCount = (vr_virtualScreenShape->integer == CURVED) ? cylinderIndexCount : quadIndexCount;

		VR_VirtualScreen_GetModelMatrix(&model, &centeredHead);

		qglUseProgram(vsShaderProgram);
		qglBindVertexArray(VAO);

		// Use explicit layout locations from shader (no runtime lookup needed)
		qglUniformMatrix4fv(UNIFORM_LOC_MODEL, 1, GL_FALSE, (float*)model.m);
		qglUniformMatrix4fv(UNIFORM_LOC_VIEW + 0, 1, GL_FALSE, (float*)view[0].m);
		qglUniformMatrix4fv(UNIFORM_LOC_VIEW + 1, 1, GL_FALSE, (float*)view[1].m);
		qglUniformMatrix4fv(UNIFORM_LOC_PROJ + 0, 1, GL_FALSE, (float*)projection[0].m);
		qglUniformMatrix4fv(UNIFORM_LOC_PROJ + 1, 1, GL_FALSE, (float*)projection[1].m);

		glBindTexture(GL_TEXTURE_2D, virtualScreenImage);
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
	}

	if (!prevBlend)
	{
		qglDisable(GL_BLEND);
	}
	if (depthMask == GL_TRUE)
	{
		qglDepthMask(GL_TRUE);
	}
	if (prevDepthTest)
	{
		qglEnable(GL_DEPTH_TEST);
	}

	glBindTexture(GL_TEXTURE_2D, previousTexture);
	qglBindVertexArray(previousVAO);
	qglUseProgram(previousProgram);
}
