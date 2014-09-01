#include "glmain.h"
#include <windows.h>
#include "winmain.h"
#include "system.h"
#include "balloffset.h"

// Define constants related to the shapes
float dist2stripe = 8;
float fovAng = 80 * M_PI / 180;

// Define offset values
float BallOffsetNow = 0.0f;

// Vertex shader which passes through the texture and position coordinates
const char* vertex_shader =
"#version 400\n"
"in vec4 position;"
"in vec2 texcoord;"
"uniform mat4 Projection;"
"uniform mat4 View;"
"uniform mat4 Model;"
"out vec2 Texcoord;"
"void main () {"
"  Texcoord = texcoord;"
"  gl_Position = Projection * View * Model * position;"
"}";

// Fragment shader which calculates the desired distortion (position and brightness) for projecting onto a cylindrical screen
const char* fragment_shader =
"#version 400\n"
"in vec2 Texcoord;"
"out vec4 frag_colour;"
"uniform float cyl;"
"uniform int projnum;"
"uniform sampler2D tex;"
"void main () {"

"  float LtoScreen = 6.55;"
"  float wofScreen = 1.93;"
"  float proj0power = 0.268;"
"  float proj1power = 0.25;"
"  float proj2power = 0.262;"
"  float projnorm = min(proj0power, min(proj1power, proj2power));"

"  float angofScreen = 4*3.141592/9;"

"  float texshifts = wofScreen*(Texcoord.s-0.5);"
"  float texshiftt = Texcoord.t-0.5;"
"  float modtfactor = (1.5 - sqrt(1.5*1.5-texshifts*texshifts))/LtoScreen + 1.0;"
"  float distorttinit = texshiftt*modtfactor + 0.5;"

"  float distortsinit = 0.3881*pow(Texcoord.s,4)-0.1844*pow(Texcoord.s,3) - 0.2624*pow(Texcoord.s,2)+1.049*Texcoord.s;"

"  float distorts = 0.5*(1+ tan(angofScreen*(distortsinit - 0.5))/tan(0.5*angofScreen));"
"  float distortt = 0.5 + (distorttinit - 0.5) * cos(0.5 * angofScreen)/cos(angofScreen*(distorts-0.5));"

"  float brightcorrect = 1.243*pow(distorts,4)-1.328*pow(distorts,3) + 0.8553*pow(distorts,2)-0.3047*distorts+0.5221;"

"  if (cyl == 0.0)"
"  {"
"   distorts = Texcoord.s;"
"   distortt = Texcoord.t;"
"   brightcorrect = 1;"
"  }"

" float projcorrect = 0.0;"
" if (projnum == 100)"
" {"
"  projcorrect = 1.0;"
" }"
" if (projnum == 0)"
" {"
"  projcorrect = projnorm/proj0power;"
" }"" if (projnum == 1)"
" {"
"  projcorrect = projnorm/proj1power;"
" }"" if (projnum == 2)"
" {"
"  projcorrect = projnorm/proj2power;"
" }"
"  vec2 distort = vec2 (distorts, distortt);"
"  vec4 unmodColor = texture(tex, distort);"
"  frag_colour = projcorrect*brightcorrect*vec4(unmodColor.r*1.0, unmodColor.g*1.0, unmodColor.b*1.0, 1.0 );"
"}";

// Define the OpenGL constants
GLuint vao;
GLuint vbo;
GLuint vs;
GLuint fs;
GLuint shader_program;
GLuint *tex;
GLuint cylLocation;
GLuint ProjNumber;
glm::mat4 ProjectionMatrix;
GLuint ProjectionID;
glm::mat4 ViewMatrix;
GLuint ViewID;
glm::mat4 ModelMatrix;
GLuint ModelID;

// InitOpenGL: initializes OpenGL; resize projection and other setup
void InitOpenGL(void)
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

	// Initialize the vertex array object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Initialize the buffer array object
	glGenBuffers(1, &vbo);

	// Define constants for defining the shapes
	float windowSpan = dist2stripe * tanf(fovAng / 2);
	float aspect = float(SCRWIDTH) / float(SCRHEIGHT);

	// Define the vertices
	float vertices[] = {
		// The first four points are the position, while the final two are the texture coordinates
		// First set of points is for the undistorted vertical stripe.
		-0.8f, 0.0f, -40.0f, 1.0f, 1.0f, 1.0f,
		-0.8f, 0.0f, 40.0f, 1.0f, 1.0f, 0.0f,
		0.8f, 0.0f, -40.0f, 1.0f, 0.0f, 1.0f,
		0.8f, 0.0f, 40.0f, 1.0f, 0.0f, 0.0f,

		// Second set of points in for the final display that the texture will be mapped onto.
		-windowSpan, 0, -windowSpan / aspect, 1.0f, 1.0f, 1.0f,
		-windowSpan, 0, windowSpan / aspect, 1.0f, 1.0f, 0.0f,
		windowSpan, 0, -windowSpan / aspect, 1.0f, 0.0f, 1.0f,
		windowSpan, 0, windowSpan / aspect, 1.0f, 0.0f, 0.0f
	};

	// Bind the vertex data to the buffer array
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Initialize the element array buffer
	GLuint ebo;
	glGenBuffers(1, &ebo);

	// Define the elements for the stripe and the projected screen.
	GLuint elements[] = {
		0, 1, 2,
		1, 2, 3,

		4, 5, 6,
		5, 6, 7,
	};

	//Bind the element data to the array.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	// Create the shaders
	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertex_shader, NULL);
	glCompileShader(vs);

	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragment_shader, NULL);
	glCompileShader(fs);

	shader_program = glCreateProgram();
	glAttachShader(shader_program, fs);
	glAttachShader(shader_program, vs);
	glLinkProgram(shader_program);
	glUseProgram(shader_program);

	// Create a pointer for the position
	GLint posAttrib = glGetAttribLocation(shader_program, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 4, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);

	// Create a pointer for the texture coordinates
	GLint texAttrib = glGetAttribLocation(shader_program, "texcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(4 * sizeof(GLfloat)));

	// Create a texture array
	tex = new GLuint[6];
	glGenTextures(6, tex);

	// Make the first texture red
	glBindTexture(GL_TEXTURE_2D, tex[0]);
	float pixelsR[] = {
		1.0f, 0.0f, 0.0f,
	};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, pixelsR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Make the second texture green
	glBindTexture(GL_TEXTURE_2D, tex[1]);
	float pixelsG[] = {
		0.0f, 1.0f, 0.0f,
	};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, pixelsG);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Make the third texture blue
	glBindTexture(GL_TEXTURE_2D, tex[2]);
	float pixelsB[] = {
		0.0f, 0.0f, 1.0f,
	};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, pixelsB);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Prep the remaining textures to be used for cylindrical distortion
	for (int n = 0; n < 3; n++) {
		glBindTexture(GL_TEXTURE_2D, tex[3 + n]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}


	// Pull out the cylindrical distortion switch uniform from the shader
	cylLocation = glGetUniformLocation(shader_program, "cyl");

	// Pull out the projector switch to set the brightness
	ProjNumber = glGetUniformLocation(shader_program, "projnum");

	// Pull out the matrices
	ProjectionID = glGetUniformLocation(shader_program, "Projection");
	ViewID = glGetUniformLocation(shader_program, "View");
	ModelID = glGetUniformLocation(shader_program, "Model");

	glViewport(0, 0, SCRWIDTH, SCRHEIGHT);
}

void RenderFrame(int windowNum, int direction)
{
	glm::mat4 identity;
	ProjectionMatrix = glm::perspective(float(360 / M_PI * atanf(tanf(fovAng / 2) * float(SCRHEIGHT) / float(SCRWIDTH))), float(SCRWIDTH) / float(SCRHEIGHT), 0.1f, 1000.0f);
	ViewMatrix = glm::lookAt(glm::vec3(0, dist2stripe, 0), glm::vec3((windowNum - 1) * dist2stripe*tanf(fovAng), 0, 0), glm::vec3(0, 0, 1)); //
	glUniformMatrix4fv(ProjectionID, 1, false, glm::value_ptr(ProjectionMatrix));
	glUniformMatrix4fv(ViewID, 1, false, glm::value_ptr(ViewMatrix));

	glUniform1f(cylLocation, (float) 0.0f); // Initially, we want an undistorted projection
	glUniform1f(ProjNumber, (int) 100);  // No brightness correction the first time

	for (int n = 0; n < 3; n++) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Set the background color to black
		glBindTexture(GL_TEXTURE_2D, tex[n]); // Bind the appropriate color texture
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers
		if (closed)
		{
			//Advance the stripe according to the ball offset
			io_mutex.lock();
			BallOffsetNow = BallOffset;
			io_mutex.unlock();
		}
		else
		{
			//Advance the stripe over time - FOR OPEN LOOP
			TimeOffset(BallOffsetNow, direction, CounterStart);
		}
		ModelMatrix = glm::translate(identity, glm::vec3(dist2stripe*sinf(BallOffsetNow * M_PI / 180), dist2stripe*(1.0f - cosf(BallOffsetNow  * M_PI / 180)), 0.0f)) * glm::rotate(identity, BallOffsetNow, glm::vec3(0.0f, 0.0f, 1.0f));
		glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(ModelMatrix));
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		// Capture the stripe as a texture, distort it for the cylindrical screen using the shader, and map it back onto a rectangle.
		glBindTexture(GL_TEXTURE_2D, tex[3 + n]);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, SCRWIDTH, SCRHEIGHT, 0);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUniform1f(cylLocation, (float) 1.0f);
	if (windowNum > 0){
		ModelMatrix = glm::translate(identity, glm::vec3(dist2stripe*sinf(fovAng * (windowNum - 1)), dist2stripe*(1.0f - cosf(fovAng * (windowNum - 1))), 0.0f)) * glm::rotate(identity, (float)(fovAng * 180 / M_PI * (windowNum - 1)), glm::vec3(0.0f, 0.0f, 1.0f));
	}
	else{
		ModelMatrix = glm::translate(identity, glm::vec3(dist2stripe*sinf(2 * M_PI - fovAng), dist2stripe*(1.0f - cosf(2 * M_PI - fovAng)), 0.0f)) * glm::rotate(identity, (float)(360 - fovAng * 180 / M_PI), glm::vec3(0.0f, 0.0f, 1.0f));
	}
	glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(ModelMatrix));
	glUniform1f(ProjNumber, (int) windowNum);  // Correct for the brightness difference between projectors

	for (int n = 0; n < 3; n++) {
		glBindTexture(GL_TEXTURE_2D, tex[3 + n]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(GLfloat)));
	}

	if (direction == 0)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void GLShutdown(void)
{
	glDeleteTextures(1, tex);
	glDeleteProgram(shader_program);
	glDeleteShader(fs);
	glDeleteShader(vs);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	CloseOffset();
}