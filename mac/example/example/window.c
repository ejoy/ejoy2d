#include <GLFW/glfw3.h>

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include "fault.h"

#include  "winfw.h"

#define UPDATE_INTERVAL 1       /* 10ms */

#define TITLE "Ejoy"
#define WIDTH 1024
#define HEIGHT 768


struct GLFW_context {
  GLFWwindow* window;
  bool is_press;
};

static struct GLFW_context *CONTEXT = NULL;

void font_init();


static uint32_t
_gettime(void) {
  uint32_t t;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  t = (uint32_t)(tv.tv_sec & 0xffffff) * 100;
  t += tv.tv_usec / 10000;
  return t;
}

static void
_glfw_error_cb(int error, const char* desc) {
  fault("glfw: %s\n", desc);
}

static void
_glfw_key_cb(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if(key == GLFW_KEY_Q && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GL_TRUE);
  }
}

static void
_glfw_pos_cb(GLFWwindow* window, double xpos, double ypos) {
  if(CONTEXT->is_press) {
    ejoy2d_win_touch(xpos, ypos, TOUCH_MOVE);
  }
}


static void 
_glfw_btn_cb(GLFWwindow* window, int button, int action, int mods) {
  CONTEXT->is_press = (action==GLFW_PRESS)?(true):(false);
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);

  switch(action) {
    case GLFW_PRESS:
      ejoy2d_win_touch(xpos, ypos, TOUCH_BEGIN);
      CONTEXT->is_press = true;
      break;
    case GLFW_RELEASE:
      ejoy2d_win_touch(xpos, ypos, TOUCH_END);
      CONTEXT->is_press = false;
      break;
    default:
      CONTEXT->is_press = false;
      break;
  }
}

static void
_glfw_init(struct GLFW_context* context) {
  if(!glfwInit()) {
    fault("glfw init error.\n");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  context->window = glfwCreateWindow(WIDTH, HEIGHT, TITLE, NULL, NULL);
  if (!context->window) {
    glfwTerminate();
    fault("glfw create window error.\n");
  }

  // set call back
  glfwSetErrorCallback(_glfw_error_cb);
  glfwSetKeyCallback(context->window, _glfw_key_cb);
  glfwSetCursorPosCallback(context->window, _glfw_pos_cb);
  glfwSetMouseButtonCallback(context->window, _glfw_btn_cb);


  glfwMakeContextCurrent(context->window);
  glfwSwapInterval(1);
}


static void
_version_info() {
  const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
  const GLubyte* version = glGetString(GL_VERSION); // version as a string
  const GLubyte* glsl_version = glGetString(GL_SHADING_LANGUAGE_VERSION); // glsl version

  printf("Renderer: %s\n", renderer);
  printf("OpenGL version supported %s\n", version);
  printf("GLSL:  %s\n", glsl_version);
}


int 
main(int argc, char *argv[]) {
  struct GLFW_context ctx_value = {
    .window = NULL,
    .is_press = false,
  };

  CONTEXT = &ctx_value;
  _glfw_init(CONTEXT);

  _version_info();

  font_init();
  ejoy2d_win_init(argc, argv);

  uint32_t timestamp = 0;
  GLFWwindow* window = CONTEXT->window;

  /* Loop until the user closes the window */
  while(!glfwWindowShouldClose(window)) {
    // update frame
    uint32_t current = _gettime();
    if(current - timestamp >= UPDATE_INTERVAL) {
      timestamp = current;
      ejoy2d_win_update();
      ejoy2d_win_frame();
    }

    /* Swap front and back buffers */
    glfwSwapBuffers(window);

    /* Poll for and process events */
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}