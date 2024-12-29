//
// Author: Ahmet Oguz Akyuz
// 
// This is a sample code that draws a single block piece at the center
// of the window. It does many boilerplate work for you -- but no
// guarantees are given about the optimality and bug-freeness of the
// code. You can use it to get started or you can simply ignore its
// presence. You can modify it in any way that you like.
//
//


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#define _USE_MATH_DEFINES
#include <math.h>
#include <GL/glew.h>
//#include <OpenGL/gl3.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp> // GL Math library header
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> 
#include <ft2build.h>
#include FT_FREETYPE_H

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;

GLuint gProgram[3];
int gWidth = 600, gHeight = 1000;
GLuint gVertexAttribBuffer, gTextVBO, gIndexBuffer;
GLuint gTex2D;
int gVertexDataSizeInBytes, gNormalDataSizeInBytes;
int gTriangleIndexDataSizeInBytes, gLineIndexDataSizeInBytes;

GLint modelingMatrixLoc[2];
GLint viewingMatrixLoc[2];
GLint projectionMatrixLoc[2];
GLint eyePosLoc[2];
GLint lightPosLoc[2];
GLint kdLoc[2];

glm::mat4 projectionMatrix;
glm::mat4 viewingMatrix;
glm::mat4 modelingMatrix = glm::translate(glm::mat4(1.f), glm::vec3(-0.5, -0.5, -0.5));
glm::vec3 eyePos = glm::vec3(0, 0, 24);
glm::vec3 lightPos = glm::vec3(0, 0, 7);

glm::vec3 kdGround(0.334, 0.288, 0.635); // this is the ground color in the demo
glm::vec3 kdCubes(0.86, 0.11, 0.31);
glm::vec3 activeBlockCenter(0, 6, 0);//top center is (0, 6, 0)
int activeProgramIndex = 0;

float gRotationAngle = 0.0f; // current rotation angle of the cube
float startingAngle = 0.0f; 
float endingAngle = 90.0f;
bool isRotatingLeft = false; //objects currently rotating
bool isRotatingRight = false;
int sideState = 0; // 0: front, 1: left, 2: back, 3: right
int speedHolder = 0;
int speedPerFrame = 0; //speed of the block, it will reset after reaching certain number and block will move down
int speedLimit = 60; //speed limit of the block
static bool cellOccupied[9][16][9] = {false}; // 9x16x9 grid, initially all cells are empty, including the floor
int score = 0;
bool gameOver = false;
std::string currentKey = "";
int keyTime = 0;

// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

// For reading GLSL files
bool ReadDataFromFile(
    const string& fileName, ///< [in]  Name of the shader file
    string&       data)     ///< [out] The contents of the file
{
    fstream myfile;

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            data += curLine;
            if (!myfile.eof())
            {
                data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    return true;
}

GLuint createVS(const char* shaderName)
{
    string shaderSource;

    string filename(shaderName);
    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shader, &length);
    glCompileShader(vs);

    char output[1024] = {0};
    glGetShaderInfoLog(vs, 1024, &length, output);
    printf("VS compile log: %s\n", output);

	return vs;
}

GLuint createFS(const char* shaderName)
{
    string shaderSource;

    string filename(shaderName);
    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shader, &length);
    glCompileShader(fs);

    char output[1024] = {0};
    glGetShaderInfoLog(fs, 1024, &length, output);
    printf("FS compile log: %s\n", output);

	return fs;
}

void initFonts(int windowWidth, int windowHeight)
{
    // Set OpenGL options
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(windowWidth), 0.0f, static_cast<GLfloat>(windowHeight));
    glUseProgram(gProgram[2]);
    glUniformMatrix4fv(glGetUniformLocation(gProgram[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    }

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 0, &face))
    //if (FT_New_Face(ft, "/usr/share/fonts/truetype/gentium-basic/GenBkBasR.ttf", 0, &face)) // you can use different fonts
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); 

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph 
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
                );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (GLuint) face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //
    // Configure VBO for texture quads
    //
    glGenBuffers(1, &gTextVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initShaders()
{
	// Create the programs

    gProgram[0] = glCreateProgram();
	gProgram[1] = glCreateProgram();
	gProgram[2] = glCreateProgram();

	// Create the shaders for both programs

    GLuint vs1 = createVS("vert.glsl"); // for cube shading
    GLuint fs1 = createFS("frag.glsl");

	GLuint vs2 = createVS("vert2.glsl"); // for border shading
	GLuint fs2 = createFS("frag2.glsl");

	GLuint vs3 = createVS("vert_text.glsl");  // for text shading
	GLuint fs3 = createFS("frag_text.glsl");

	// Attach the shaders to the programs

	glAttachShader(gProgram[0], vs1);
	glAttachShader(gProgram[0], fs1);

	glAttachShader(gProgram[1], vs2);
	glAttachShader(gProgram[1], fs2);

	glAttachShader(gProgram[2], vs3);
	glAttachShader(gProgram[2], fs3);

	// Link the programs

    for (int i = 0; i < 3; ++i)
    {
        glLinkProgram(gProgram[i]);
        GLint status;
        glGetProgramiv(gProgram[i], GL_LINK_STATUS, &status);

        if (status != GL_TRUE)
        {
            cout << "Program link failed: " << i << endl;
            exit(-1);
        }
    }


	// Get the locations of the uniform variables from both programs

	for (int i = 0; i < 2; ++i)
	{
		modelingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "modelingMatrix");
		viewingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "viewingMatrix");
		projectionMatrixLoc[i] = glGetUniformLocation(gProgram[i], "projectionMatrix");
		eyePosLoc[i] = glGetUniformLocation(gProgram[i], "eyePos");
		lightPosLoc[i] = glGetUniformLocation(gProgram[i], "lightPos");
		kdLoc[i] = glGetUniformLocation(gProgram[i], "kd");

        glUseProgram(gProgram[i]);
        glUniformMatrix4fv(modelingMatrixLoc[i], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
        glUniform3fv(eyePosLoc[i], 1, glm::value_ptr(eyePos));
        glUniform3fv(lightPosLoc[i], 1, glm::value_ptr(lightPos));
        glUniform3fv(kdLoc[i], 1, glm::value_ptr(kdCubes));
	}
}

// VBO setup for drawing a cube and its borders
void initVBO()
{
    GLuint vao;
    glGenVertexArrays(1, &vao);
    assert(vao > 0);
    glBindVertexArray(vao);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	assert(glGetError() == GL_NONE);

	glGenBuffers(1, &gVertexAttribBuffer);
	glGenBuffers(1, &gIndexBuffer);

	assert(gVertexAttribBuffer > 0 && gIndexBuffer > 0);

	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

    GLuint indices[] = {
        0, 1, 2, // front
        3, 0, 2, // front
        4, 7, 6, // back
        5, 4, 6, // back
        0, 3, 4, // left
        3, 7, 4, // left
        2, 1, 5, // right
        6, 2, 5, // right
        3, 2, 7, // top
        2, 6, 7, // top
        0, 4, 1, // bottom
        4, 5, 1  // bottom
    };

    GLuint indicesLines[] = {
        7, 3, 2, 6, // top
        4, 5, 1, 0, // bottom
        2, 1, 5, 6, // right
        5, 4, 7, 6, // back
        0, 1, 2, 3, // front
        0, 3, 7, 4, // left
    };

    GLfloat vertexPos[] = {
        0, 0, 1, // 0: bottom-left-front
        1, 0, 1, // 1: bottom-right-front
        1, 1, 1, // 2: top-right-front
        0, 1, 1, // 3: top-left-front
        0, 0, 0, // 0: bottom-left-back
        1, 0, 0, // 1: bottom-right-back
        1, 1, 0, // 2: top-right-back
        0, 1, 0, // 3: top-left-back
    };

    GLfloat vertexNor[] = {
         1.0,  1.0,  1.0, // 0: unused
         0.0, -1.0,  0.0, // 1: bottom
         0.0,  0.0,  1.0, // 2: front
         1.0,  1.0,  1.0, // 3: unused
        -1.0,  0.0,  0.0, // 4: left
         1.0,  0.0,  0.0, // 5: right
         0.0,  0.0, -1.0, // 6: back 
         0.0,  1.0,  0.0, // 7: top
    };

	gVertexDataSizeInBytes = sizeof(vertexPos);
	gNormalDataSizeInBytes = sizeof(vertexNor);
    gTriangleIndexDataSizeInBytes = sizeof(indices);
    gLineIndexDataSizeInBytes = sizeof(indicesLines);
    int allIndexSize = gTriangleIndexDataSizeInBytes + gLineIndexDataSizeInBytes;

	glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes + gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexPos);
	glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, gNormalDataSizeInBytes, vertexNor);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, allIndexSize, 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, gTriangleIndexDataSizeInBytes, indices);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, gTriangleIndexDataSizeInBytes, gLineIndexDataSizeInBytes, indicesLines);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));
}

void init() 
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // polygon offset is used to prevent z-fighting between the cube and its borders
    glPolygonOffset(0.5, 0.5);
    glEnable(GL_POLYGON_OFFSET_FILL);

    initShaders();
    initVBO();
    initFonts(gWidth, gHeight);
}



void renderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    // Activate corresponding render state	
    glUseProgram(gProgram[2]);
    glUniform3f(glGetUniformLocation(gProgram[2], "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },            
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }           
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

        //glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)

        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}


void reshape(GLFWwindow* window, int w, int h)
{
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;

    gWidth = w;
    gHeight = h;

    glViewport(0, 0, w, h);

	// Use perspective projection

	float fovyRad = (float) (45.0 / 180.0) * M_PI;
	projectionMatrix = glm::perspective(fovyRad, gWidth / (float) gHeight, 1.0f, 100.0f);

    // always look toward (0, 0, 0)
	viewingMatrix = glm::lookAt(eyePos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));


    for (int i = 0; i < 2; ++i)
    {
        glUseProgram(gProgram[i]);
        glUniformMatrix4fv(projectionMatrixLoc[i], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniformMatrix4fv(viewingMatrixLoc[i], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
    }
}

void drawCube(bool isFloor)
{
    glm::mat4 oldMat = modelingMatrix;
    if (isFloor) {
        glm::mat4 scaledMat = modelingMatrix * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 0.5f, 1.0f));
        modelingMatrix = scaledMat;
    }
    glUseProgram(gProgram[0]);
    glm::mat4 matR = glm::rotate(glm::mat4(1.0), glm::radians(gRotationAngle), glm::vec3(0, 1, 0));
    modelingMatrix = matR * modelingMatrix;
    //rotation should be applied as last operation, therefore it is the leftmost operation of overall


    glUniformMatrix4fv(projectionMatrixLoc[0], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(viewingMatrixLoc[0], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
    glUniformMatrix4fv(modelingMatrixLoc[0], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
    glUniform3fv(eyePosLoc[0], 1, glm::value_ptr(eyePos));

    if (isFloor)
        glUniform3fv(kdLoc[0], 1, glm::value_ptr(kdGround));
    else
        glUniform3fv(kdLoc[0], 1, glm::value_ptr(kdCubes));

    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    modelingMatrix = oldMat; // restore the modeling matrix
}

void drawCubeEdges(bool isFloor)
{
    glm::mat4 oldMat = modelingMatrix;
    if (isFloor) {
        glm::mat4 scaledMat = modelingMatrix * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 0.5f, 1.0f));
        modelingMatrix = scaledMat;
    }
    glm::mat4 matR = glm::rotate(glm::mat4(1.0), glm::radians(gRotationAngle), glm::vec3(0, 1, 0));
    modelingMatrix = matR * modelingMatrix;

    glLineWidth(3);

	glUseProgram(gProgram[1]);
    glUniformMatrix4fv(projectionMatrixLoc[1], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
    glUniformMatrix4fv(viewingMatrixLoc[1], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
    glUniformMatrix4fv(modelingMatrixLoc[1], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
    glUniform3fv(eyePosLoc[1], 1, glm::value_ptr(eyePos));

    glm::vec3 white(1.0, 1.0, 1.0);
    glUniform3fv(kdLoc[1], 1, glm::value_ptr(white));

    for (int i = 0; i < 6; ++i)
    {
	    glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, BUFFER_OFFSET(gTriangleIndexDataSizeInBytes + i * 4 * sizeof(GLuint)));
    }
    modelingMatrix = oldMat; // restore the modeling matrix
}

void drawTetrisBlock(glm::vec3 center) {
    for (int x = -1; x < 2; ++x) {
        for (int y = -1; y < 2; ++y) {
            for (int z = -1; z < 2; ++z) {
                glm::mat4 matT = glm::translate(glm::mat4(1.0), glm::vec3(center.x+x - 0.5, center.y+y, center.z+z - 0.5));
                modelingMatrix = matT;
                drawCube(false);
                drawCubeEdges(false);
            }
        }
    }
}

bool canMoveBlockDown(glm::vec3 tempTetrisBlockCenter) {
        //for each cell of the block, mark the cell as occupied
        //if(activeTetrisBlockCenter.y == -6)
            //return false;
        for(int x = tempTetrisBlockCenter.x - 1; x < tempTetrisBlockCenter.x + 2; x++){
            for(int y = tempTetrisBlockCenter.y - 1; y < tempTetrisBlockCenter.y + 2; y++){
                for(int z = tempTetrisBlockCenter.z - 1; z < tempTetrisBlockCenter.z + 2; z++){
                    if(cellOccupied[x+4][y+8][z+4]) //if any cell is occupied, it cannot move down
                        return false;
                }
            }
        }
    
    return true;
    
}

void checkTetrisBlockValidationLeftRightBoundary(int change) {//left right move event from user, checking w.r.t. center of block
    glm::vec3 tempCenter = {activeBlockCenter.x, activeBlockCenter.y, activeBlockCenter.z};
    switch (sideState)
    {
        case 0: //front
            if(activeBlockCenter.x + change >= -3 && activeBlockCenter.x + change <= 3)
            {
                tempCenter.x += change;
                if (canMoveBlockDown(tempCenter))
                    activeBlockCenter.x += change;
            }
            break;
        case 1: //left
            if(activeBlockCenter.z + change >= -3 && activeBlockCenter.z + change <= 3)
            {
                tempCenter.z += change;
                if (canMoveBlockDown(tempCenter))
                    activeBlockCenter.z += change;
            }
            break;
        case 2: //back
            //incrementing x w.r.t back side actually means decrementing x w.r.t. front side
            if(activeBlockCenter.x - change >= -3 && activeBlockCenter.x - change <= 3) {
                tempCenter.x -= change;
                if (canMoveBlockDown(tempCenter))
                    activeBlockCenter.x -= change;
            }
            break;
        case 3: //right
            //incrementing z w.r.t right side actually means decrementing z w.r.t. left side
            if(activeBlockCenter.z - change >= -3 && activeBlockCenter.z - change <= 3) {
                tempCenter.z -= change;
                if (canMoveBlockDown(tempCenter))
                    activeBlockCenter.z -= change;
            }
            break;
        default:
            break;
    }
}

void display()
{
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    modelingMatrix = glm::mat4(1.0);
    speedHolder += speedPerFrame;
    keyTime++;

    if (keyTime > 40) {
        currentKey = "";
        keyTime = 0;
    }

    if (!gameOver) {
        glm::vec3 tempCenter = {activeBlockCenter.x, activeBlockCenter.y-1, activeBlockCenter.z};
        if (speedHolder >= speedLimit) {
            if(canMoveBlockDown(tempCenter)) {
                activeBlockCenter.y -= 1;
                speedHolder = 0;
            }
            else{
                for(int x = activeBlockCenter.x - 1; x < activeBlockCenter.x + 2; x++){
                    for(int y = activeBlockCenter.y - 1; y < activeBlockCenter.y + 2; y++){
                        for(int z = activeBlockCenter.z - 1; z < activeBlockCenter.z + 2; z++){
                            cellOccupied[x+4][y+8][z+4] = true;
                        }
                    }
                }
                activeBlockCenter = {0, 6, 0};
                if (!canMoveBlockDown(activeBlockCenter))
                {
                    gameOver = true;
                }       
            }
            speedHolder = 0;
        }

        //rotation smoothing
        if(isRotatingLeft){
            gRotationAngle += 3.0f;
            if(gRotationAngle >= endingAngle){
                gRotationAngle = startingAngle + 90.0f;
                isRotatingLeft = false;
            }
            if(gRotationAngle >= 360.0f)
                gRotationAngle -= 360.0f;
        }
        if(isRotatingRight){
            gRotationAngle -= 3.0f;
            if(gRotationAngle <= endingAngle){
                gRotationAngle = startingAngle - 90.0f;
                isRotatingRight = false;
            }
            if(gRotationAngle <= 0.0f){
                gRotationAngle = 360.0f;
            }
                
        }

        // draw active block
        drawTetrisBlock(activeBlockCenter);
    } else {
        renderText("Game Over!", gWidth / 2 - 175, gHeight / 2, 1.5, glm::vec3(1, 1, 0));
    }
    
    // fill occupied array for floor
    for(int x = -4; x < 5; x++){
        for(int z = -4; z < 5; z++){
            cellOccupied[x+4][0][z+4] = true;
        }
    }

    // draw floor
    for(int x = -4; x < 5; x++){
        for(int z = -4; z < 5; z++){
            glm::mat4 matT = glm::translate(glm::mat4(1.0), glm::vec3(x - 0.5, -7.5, z - 0.5));
            modelingMatrix =  matT; 

            drawCube(true);
            drawCubeEdges(true);
        }
    }

    // draw blocks on the ground
    for (int x = 0; x < 9; x++) {
        for (int y = 1; y < 16; y++) {
            for (int z = 0; z < 9; z++) {
                if (cellOccupied[x][y][z]) {
                    glm::mat4 matT = glm::translate(glm::mat4(1.0), glm::vec3(x - 4.5, y - 8, z - 4.5));
                    modelingMatrix = matT;
                    drawCube(false);
                    drawCubeEdges(false);
                }
            }
        }
    }

    // check if there is a complete row, if so, remove the row and move the blocks above down and increase the score
    for (int y = 1; y < 16; y += 3) {
        for (int x = 0; x < 9; x++) {
            for (int z = 0; z < 9; z++) {
                if (!cellOccupied[x][y][z]) {
                    goto next;
                }
            }
        }

        score += 243;

        for (int x = 0; x < 9; x++) {
            for (int _y = y; _y < y + 3; _y++) {
                for (int z = 0; z < 9; z++) {
                    cellOccupied[x][_y][z] = false;
            }
            }
        }

        for (int _y = y; _y < 16; _y++) {
            for (int x = 0; x < 9; x++) {
                for (int z = 0; z < 9; z++) {
                    if (_y + 3 < 16)
                        cellOccupied[x][_y][z] = cellOccupied[x][_y + 3][z];
                    else
                        cellOccupied[x][_y][z] = false;
                }
            }
        }

        next:;
    }

    // draw camera text
    switch (sideState) {
        case 0: { renderText("Front", gWidth * 0.1 + 10, (gHeight - 50), 0.5, glm::vec3(1, 1, 0)); break; }
        case 1: { renderText("Left", gWidth * 0.1 + 10, (gHeight - 50), 0.5, glm::vec3(1, 1, 0)); break; }
        case 2: { renderText("Back", gWidth * 0.1 + 10, (gHeight - 50), 0.5, glm::vec3(1, 1, 0)); break; }
        case 3: { renderText("Right", gWidth * 0.1 + 10, (gHeight - 50), 0.5, glm::vec3(1, 1, 0)); break; }
    }

    // draw key text
    renderText(currentKey, gWidth * 0.1 + 10, (gHeight - 100), 0.5, glm::vec3(1, 0, 0));

    // draw score text
    std::ostringstream oss;
    oss << "Score: " << score;
    std::string scoreText = oss.str();
    renderText(scoreText, gWidth - 150, (gHeight - 50), 0.5, glm::vec3(1, 1, 0));

    assert(glGetError() == GL_NO_ERROR);
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if ((key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    else if (!gameOver && key == GLFW_KEY_A && action == GLFW_PRESS)
	{
		checkTetrisBlockValidationLeftRightBoundary(-1);
        keyTime = 0;
        currentKey = "A";
	}
    else if (!gameOver && key == GLFW_KEY_D && action == GLFW_PRESS)
    {
        checkTetrisBlockValidationLeftRightBoundary(1);
        keyTime = 0;
        currentKey = "D";
    }
    else if (!gameOver && key == GLFW_KEY_H && action == GLFW_PRESS) {
        if (!isRotatingLeft && !isRotatingRight) {
            if(gRotationAngle == 360.0f)
                gRotationAngle = 0.0f;
            startingAngle = gRotationAngle;
            endingAngle = gRotationAngle + 90.0f;
            isRotatingLeft = true;
            sideState = (sideState + 1) % 4;

            keyTime = 0;
            currentKey = "H";
        }
    }
    else if (!gameOver && key == GLFW_KEY_K && action == GLFW_PRESS) {
        if (!isRotatingRight && !isRotatingLeft) {
            if(gRotationAngle == 0.0f)
                gRotationAngle = 360.0f;
            startingAngle = gRotationAngle;
            endingAngle = gRotationAngle - 90.0f;
            isRotatingRight = true;

            sideState--;
            if (sideState < 0)
                sideState += 4;

            keyTime = 0;
            currentKey = "K";
        }
    }
    else if (!gameOver && key == GLFW_KEY_S && action == GLFW_PRESS) {
        speedPerFrame += 1;

        keyTime = 0;
        currentKey = "S";
    }
    else if (!gameOver && key == GLFW_KEY_W && action == GLFW_PRESS) {
        if(speedPerFrame > 0)
            speedPerFrame -= 1;

        keyTime = 0;
        currentKey = "W";
    }
}

void mainLoop(GLFWwindow* window)
{
    while (!glfwWindowShouldClose(window))
    {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{
    GLFWwindow* window;
    if (!glfwInit())
    {
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(gWidth, gHeight, "tetrisGL", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    char rendererInfo[512] = {0};
    strcpy(rendererInfo, (const char*) glGetString(GL_RENDERER));
    strcat(rendererInfo, " - ");
    strcat(rendererInfo, (const char*) glGetString(GL_VERSION));
    glfwSetWindowTitle(window, rendererInfo);

    init();

    glfwSetKeyCallback(window, keyboard);
    glfwSetWindowSizeCallback(window, reshape);

    reshape(window, gWidth, gHeight); // need to call this once ourselves
    mainLoop(window); // this does not return unless the window is closed

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
