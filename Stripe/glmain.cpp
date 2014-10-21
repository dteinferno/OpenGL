////////////////////////////////////////////////////////////////////////////////////
//                 Main Routine for Drawing Objects via OpenGL                    //
////////////////////////////////////////////////////////////////////////////////////
//                           Dan Turner-Evans                                     //
//                          V0.0 - 10/15/2014                                     //
////////////////////////////////////////////////////////////////////////////////////

#include "glmain.h"
#include <windows.h>
#include "winmain.h"
#include "system.h"
#include "balloffset.h"

// Define constants related to the shapes
float dist2stripe = 8;
float fovAng = 80 * M_PI / 180;

// Define offset values (Rotational, Forward, and Lateral)
float BallOffsetRotNow = 0.0f;
float BallOffsetForNow = 0.0f;
float BallOffsetSideNow = 0.0f;

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

"  float texshifts = wofScreen*(3*Texcoord.s-1.5);"
" if (Texcoord.s < 0.3333)"
"  texshifts = wofScreen*(3*Texcoord.s-0.5);"
" if (Texcoord.s > 0.6666)"
"  texshifts = wofScreen*(3*Texcoord.s-2.5);"

"  float texshiftt = Texcoord.t-0.5;"
"  float modtfactor = (1.5 - sqrt(1.5*1.5-texshifts*texshifts))/LtoScreen + 1.0;"
"  float distorttinit = texshiftt*modtfactor + 0.5;"

"  float texchop = 3*Texcoord.s;"
" if (Texcoord.s > 0.3333)"
"  texchop = texchop - 1;"
" if (Texcoord.s > 0.6666)"
"  texchop = texchop - 1;"

"  float distortsinit = 0.3881*pow(texchop,4)-0.1844*pow(texchop,3) - 0.2624*pow(texchop,2)+1.049*texchop;"

"  float distorts = 0.5*(1+ tan(angofScreen*(distortsinit - 0.5))/tan(0.5*angofScreen));"
"  float distortt = 0.5 + (distorttinit - 0.5) * cos(0.5 * angofScreen)/cos(angofScreen*(distorts-0.5));"

"  distorts = 0.333 * distorts; "
" if (Texcoord.s > 0.3333)"
"  distorts = distorts + 0.333;"
" if (Texcoord.s > 0.6666)"
"  distorts = distorts + 0.333;"

"  distortt = distorttinit;"

"  float brightcorrect = 1.243*pow(distorts,4)-1.328*pow(distorts,3) + 0.8553*pow(distorts,2)-0.3047*distorts+0.5221;"

"  if (cyl == 0.0)"
"  {"
"   distorts = Texcoord.s;"
"   distortt = Texcoord.t;"
"   brightcorrect = 1;"
"  }"

" float projcorrect = projnorm/proj1power;"
" if (Texcoord.s < 0.3333)"
"  projcorrect = projnorm/proj0power;"
" if (Texcoord.s > 0.6666)"
"  projcorrect = projnorm/proj2power;"

" if (projnum == 100)"
" {"
"  projcorrect = 1.0;"
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

// InitOpenGL: initializes OpenGL; defines buffers, constants, etc...
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
	float windowSpan =  dist2stripe * tanf(fovAng / 2);
	float aspect = float(SCRWIDTH) / float(SCRHEIGHT);

	// Define the vertices
	float vertices[] = {
		// The first four points are the position, while the final two are the texture coordinates
		// First set of points is for the undistorted vertical stripe.
		-0.8f, 0.0f, -40.0f, 1.0f, 1.0f, 1.0f,
		-0.8f, 0.0f, 40.0f, 1.0f, 1.0f, 0.0f,
		0.8f, 0.0f, -40.0f, 1.0f, 0.0f, 1.0f,
		0.8f, 0.0f, 40.0f, 1.0f, 0.0f, 0.0f,

		// Second set of points for the final display that the texture will be mapped onto.
		-windowSpan, 0, -windowSpan / aspect, 1.0f, 1.0f, 1.0f,
		-windowSpan, 0, windowSpan / aspect, 1.0f, 1.0f, 0.0f,
		windowSpan, 0, -windowSpan / aspect, 1.0f, 0.0f, 1.0f,
		windowSpan, 0, windowSpan / aspect, 1.0f, 0.0f, 0.0f,

		// Third set of points for a blinking dot that will trigger the photodiode.
		0.95*windowSpan, 0, windowSpan * ( 1 / aspect - 0.05), 1.0f, 1.0f, 1.0f,
		0.95*windowSpan, 0, windowSpan / aspect, 1.0f, 1.0f, 0.0f,
		windowSpan, 0, windowSpan * (1 / aspect - 0.05), 1.0f, 0.0f, 1.0f,
		windowSpan, 0, windowSpan / aspect, 1.0f, 0.0f, 0.0f,
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

		8, 9, 10,
		9, 10, 11,
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
	tex = new GLuint[7];
	glGenTextures(7, tex);

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

	// Make the final texture white (for the photodiode)
	glBindTexture(GL_TEXTURE_2D, tex[6]);
	float pixelsW[] = {
		1.0f, 1.0f, 1.0f,
	};
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT, pixelsW);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Pull out the cylindrical distortion switch uniform from the shader
	cylLocation = glGetUniformLocation(shader_program, "cyl");

	// Pull out the projector switch to set the brightness
	ProjNumber = glGetUniformLocation(shader_program, "projnum");

	// Pull out the matrices
	ProjectionID = glGetUniformLocation(shader_program, "Projection");
	ViewID = glGetUniformLocation(shader_program, "View");
	ModelID = glGetUniformLocation(shader_program, "Model");

}

void RenderFrame(int direction)
{
	glm::mat4 identity;

	// Map the desired image onto the three different colors
	for (int n = 0; n < 3; n++) {

		//Clear the image and bind the appropriate color texture
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Set the background color to black
		glBindTexture(GL_TEXTURE_2D, tex[n]); // Bind the appropriate color texture
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers

		glUniform1f(cylLocation, (float) 0.0f); // Initially, we want an undistorted projection
		glUniform1f(ProjNumber, (int)100);  // No brightness correction the first time

		// Take a picture for each of the three camera angles
		for (int windowNum = 0; windowNum < 3; windowNum++) {

			// Define the scene to be captured
			glViewport(SCRWIDTH*windowNum / 3, 0, SCRWIDTH / 3, SCRHEIGHT); // Restrict the viewport to the region of interest
			ProjectionMatrix = glm::perspective(float(360 / M_PI * atanf(tanf(fovAng / 2) * float(SCRHEIGHT) / float(SCRWIDTH))), float(SCRWIDTH) / float(SCRHEIGHT), 0.1f, 1000.0f); //Set the perspective for the projector
			ViewMatrix = glm::lookAt(glm::vec3(0, dist2stripe, 0), glm::vec3(-(windowNum - 1) * dist2stripe*tanf(fovAng), 0, 0), glm::vec3(0, 0, 1)); //Look at the appropriate direction for the projector
			glUniformMatrix4fv(ProjectionID, 1, false, glm::value_ptr(ProjectionMatrix));
			glUniformMatrix4fv(ViewID, 1, false, glm::value_ptr(ViewMatrix));

			if (closed) // Advance the environment according to the ball movement
			{
				io_mutex.lock();
				BallOffsetRotNow =  BallOffsetRot;
				BallOffsetForNow =  BallOffsetFor;
				BallOffsetSideNow = BallOffsetSide;
				io_mutex.unlock();
			}
			else  // Advance the environment according to open loop parameters
			{
				TimeOffset(BallOffsetRotNow, direction, CounterStart);
				BallOffsetForNow = 0.0f;
				BallOffsetSideNow = 0.0f;
			}

			// Apply the movement
			ModelMatrix = glm::translate(identity, glm::vec3(BallOffsetSideNow, BallOffsetForNow, 0.0f)) *
				glm::translate(identity, glm::vec3(dist2stripe*sinf(BallOffsetRotNow * M_PI / 180), dist2stripe*(1.0f - cosf(BallOffsetRotNow  * M_PI / 180)), 0.0f)) *
				glm::rotate(identity, BallOffsetRotNow, glm::vec3(0.0f, 0.0f, 1.0f));
			glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(ModelMatrix));

			// Draw the shapes
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		}
		// Capture the stripe as a texture
		glBindTexture(GL_TEXTURE_2D, tex[3 + n]);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, SCRWIDTH, SCRHEIGHT, 0);
	}


	// Map the textures onto a rectangle/window that spans all three projectors and apply the appropriate shader distortion corrections
	{
		// Allow the different colors/textures to be blended
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		// Clear the screen and apply
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//
		glUniform1f(cylLocation, (float) 1.0f); // Allow the projection to be distorted for the cylindrical screen using the shader
		glViewport(0, 0, SCRWIDTH, SCRHEIGHT); // Open up the viewport to the full screen
		ViewMatrix = glm::lookAt(glm::vec3(0, dist2stripe, 0), glm::vec3(0, 0, 0), glm::vec3(0, 0, 1)); //Look at the center of the rectangle
		glUniformMatrix4fv(ViewID, 1, false, glm::value_ptr(ViewMatrix));
		glUniformMatrix4fv(ModelID, 1, false, glm::value_ptr(identity));
		glUniform1f(ProjNumber, 1);  // Correct for the brightness difference between projectors

		// Draw the rectangle
		for (int n = 0; n < 3; n++) {
			glBindTexture(GL_TEXTURE_2D, tex[3 + n]);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(6 * sizeof(GLfloat)));
		}

	}

	if (direction == 0)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw a box to trigger the photodiode
		glBindTexture(GL_TEXTURE_2D, tex[6]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)(12 * sizeof(GLfloat)));

}


// Clear the buffers on shutdown
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