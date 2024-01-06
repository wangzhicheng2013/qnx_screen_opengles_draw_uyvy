#pragma once
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include "qnx_screen_display.hpp"
enum image_format_type : uint8_t {
    UYVY,
    NV12,
    NV21,
};

struct egl_conf_attr {
	EGLint surface_type = EGL_WINDOW_BIT;   // ask for displayable and pbuffer surfaces
	EGLint red_size = 8;                    // minimum number of red bits per pixel
	EGLint green_size = 8;                  // minimum number of green bits per pixel
	EGLint blue_size = 8;                   // minimum number of blue bits per pixel
	EGLint alpha_size = 8;                  // minimum number of alpha bits per pixel
	EGLint samples = EGL_DONT_CARE;         // minimum number of samples per pixel 
	EGLConfig config_id = 0;                // used to get a specific EGL config
};

static const char *uyvy_vertex_shader_source = "#version 300 es\n"
	"layout (location = 0) in vec2 aPos;\n"
	"layout (location = 1) in vec2 aTex;\n"
	"out vec2 fTex;\n"
	"void main()\n"
	"{\n"
	"   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
	"   fTex = aTex;\n"
	"}\0";
static const char *uyvy_fragment_shader_source = "#version 300 es\n"
	"precision highp float;\n"
	"out vec4 FragColor;\n"
	"layout (location = 0) in vec2 fTex;\n"
	"uniform sampler2D uyvySam;\n"
	"void main()\n"
	"{\n"
	"   vec3 yuv = vec3(0.0);\n"
	"   vec4 rgba = texture(uyvySam, fTex);\n"
	"   ivec2  texSize = textureSize(uyvySam, 0);\n"
	"   int xIndex = int(fTex.x * float(texSize.x) * 2.0);\n"
	"   if(xIndex % 2 == 1){\n"
	"       yuv.x = rgba.a - 0.0625;\n"
	"   }else {\n"
	"       yuv.x = rgba.g - 0.0625;\n"
	"   }\n"
	"   yuv.y = rgba.r - 0.5;\n"
	"   yuv.z = rgba.b - 0.5;\n"
	"   vec3 rgb = mat3(1.164,1.164,1.164,   0,-0.391,2.0253,   1.6019 ,-0.813,0) * yuv;\n" 
	"   FragColor = vec4(rgb, 1.0f);\n"
	"}\n\0";

static const float vertices[] = {
        // pos                      // tex
        -1.f, 1.f,                  0.0f, 0.0f,
        -1.f, -1.f,                 0.f, 1.f,
        1.f, -1.0f,                 1.0f, 1.0f,
        1.f, 1.0f,                  1.0f, 0.0f
    };
	
class qnx_screen_opengles_render_image {
private:
    int image_width_ = 1920;
    int image_height_ = 1080;
    uint8_t image_format_ = UYVY;
    egl_conf_attr egl_conf_attr_sel_;
	EGLDisplay egl_disp_ = EGL_NO_DISPLAY;
	EGLContext egl_context_ = EGL_NO_CONTEXT;
	qnx_screen_display qnx_screen_display_;
	EGLSurface egl_surf_ = EGL_NO_SURFACE;
	GLuint vertex_shader_ = 0;
	GLuint fragment_shader_ = 0;
	GLuint program_ = 0;
	unsigned int vbo_ = 0, vao_ = 0;
	GLuint texid_ = 0;
	int location_ = -1;
public:
    qnx_screen_opengles_render_image(int width = 1920, 
									 int height = 1080, 
									 uint8_t format = UYVY) : image_width_(width),
									 						   image_height_(height),
															   image_format_(format) {
		if (false == init()) {
			LOG_E("opengles render init failed!");
        	exit(0);
		}
		LOG_I("opengles render init success!");
	}
    virtual ~qnx_screen_opengles_render_image() {
		uninit();
    }
public:
    inline int get_image_size() {
        if (UYVY == image_format_) {
            return 2 * image_width_ * image_height_;
        }
        else {
            return image_width_ * image_height_ * 3 / 2;
        }
    }
	bool init() {
		if (false == create_egl_display()) {
			return false;
		}
		if (false == select_egl_config(egl_disp_)) {
			return false;
		}
		if (false == create_egl_context()) {
			return false;
		}
		int ret = qnx_screen_display_.init();
		if (ret) {
			LOG_E("qnx screen init failed:%d", ret);
			return false;
		}
		if (false == make_egl_surface()) {
			return false;
		}
		if (false == create_vertex_shader()) {
			return false;
		}
		if (false == create_fragment_shader()) {
			return false;
		}
		if (false == create_program()) {
			return false;
		}
		set_vbo_vao();
		make_texid();
		if (false == get_location()) {
			return false;
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		glUseProgram(program_);
		glBindVertexArray(vao_); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform1i(location_, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texid_);
		return true;
	}
	void uninit() {
		qnx_screen_display_.uninit();
		if (vertex_shader_ > 0) {
			glDeleteShader(vertex_shader_);
			vertex_shader_ = 0;
			LOG_I("glDeleteShader!");
		}
		if (fragment_shader_ > 0) {
			glDeleteShader(fragment_shader_);
			fragment_shader_ = 0;
			LOG_I("glDeleteShader!");
		}
		if (program_ > 0) {
			glDeleteProgram(program_);
			program_ = 0;
			LOG_I("glDeleteProgram!");
		}
		if (vao_ != 0) {
			glDeleteVertexArrays(1, &vao_);
			vao_ = 0;
			LOG_I("glDeleteVertexArrays!");
		}
		if (vbo_ != 0) {
			glDeleteBuffers(1, &vbo_);
			vbo_= 0;
			LOG_I("glDeleteBuffers!");
		}
		glDeleteTextures(1, &texid_);
		if (egl_disp_ != EGL_NO_DISPLAY && egl_surf_ != EGL_NO_SURFACE) {
			eglDestroySurface(egl_disp_, egl_surf_);
			egl_surf_ = EGL_NO_SURFACE;
			LOG_I("eglDestroySurface!");
		}
		if (egl_disp_ != EGL_NO_DISPLAY && egl_context_ != EGL_NO_CONTEXT) {
			eglDestroyContext(egl_disp_, egl_context_);
			egl_context_ = EGL_NO_CONTEXT;
			LOG_I("eglDestroyContext!");
		}
		if (egl_disp_ != EGL_NO_DISPLAY) {
			eglTerminate(egl_disp_);
			egl_disp_ = EGL_NO_DISPLAY;
			LOG_I("eglTerminate!");
		}
		LOG_I("opengles uninit ok!");
	}
	bool compile_is_ok(int shader) {
		GLint compiled_ok = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled_ok);
		if (0 == compiled_ok) {
			GLint info_log_length = 0;
			glGetShaderiv(vertex_shader_, GL_INFO_LOG_LENGTH, &info_log_length);
			char log[info_log_length + 1] = { 0 };
			glGetShaderInfoLog(vertex_shader_, info_log_length, nullptr, log);
			LOG_E("compile error:%s", log);
			return false;
		}
		return true;
	}
	bool program_link_ok(int program) {
		// check link status
		GLint link_status = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &link_status);
		if (GL_FALSE == link_status) {
			LOG_E("failed to link program!");
			return false;
		}
		return true;
	}
	void make_texid() {
		glGenTextures(1, &texid_);
		glBindTexture(GL_TEXTURE_2D, texid_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	bool get_location() {
		if (UYVY == image_format_) {
			location_ = glGetUniformLocation(program_, "uyvySam");
			if (location_ < 0) {
				LOG_E("get uyvy location failed!");
				return false;
			}
		}
		return true;
	}
	void render_image(unsigned char* image) {
		if (UYVY == image_format_) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width_ >> 1, image_height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		}
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		// post changes 
		int ret = eglSwapBuffers(egl_disp_, egl_surf_);
		if (EGL_FALSE == ret) {
			LOG_E("failed to swap buffers!");
		}
	}
private:
	bool create_egl_display() {
		egl_disp_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		if (EGL_NO_DISPLAY == egl_disp_) {
			LOG_E("no display connection matched EGL_DEFAULT_DISPLAY!");
			return false;
		}
		EGLint major = 0, minor = 0;
		// initialize EGL display
		EGLBoolean ret = eglInitialize(egl_disp_, &major, &minor);
		if (EGL_FALSE == ret) {
			LOG_E("EGL initialization failed!");
			return false;
		}
		LOG_I("EGL version:%d.%d initialization success!", major, minor);
		return true;
	}
    // query for the best EGL configuration
    bool select_egl_config(EGLDisplay& egl_display) {
        if (egl_conf_attr_sel_.config_id) {
            return true;
        }
	    EGLint egl_num_configs = 0;
		// get available EGL configurations
        EGLBoolean ret = eglGetConfigs(egl_display, nullptr, 0, &egl_num_configs);
		if (EGL_FALSE == ret) {
            LOG_E("failed to get EGL configurations!");
            return false;
		}
        LOG_I("get %d available configurations!", egl_num_configs);
        EGLConfig *egl_configs = (EGLConfig *)malloc(egl_num_configs * sizeof(EGLConfig));
        if (nullptr == egl_configs) {
            LOG_E("malloc failed!");
            exit(-1);
        }
		ret = eglGetConfigs(egl_display, egl_configs, egl_num_configs, &egl_num_configs);
		if (EGL_FALSE == ret) {
			LOG_E("failed to get list of EGL configurations!");
            free(egl_configs);
            return false; 
		}
		// loop through available configurations to select the best match
		for (int i = 0;i < egl_num_configs;i++) {
			EGLint egl_val = 0;
			// get surface type
			if (egl_conf_attr_sel_.surface_type != EGL_DONT_CARE) {
                ret = eglGetConfigAttrib(egl_display, egl_configs[i], EGL_SURFACE_TYPE, &egl_val);
				if (EGL_FALSE == ret) {
                    LOG_E("failed to get EGL surface type on config:%d", i);
                    continue;
				} 
                else if ((egl_val & egl_conf_attr_sel_.surface_type) != egl_conf_attr_sel_.surface_type){
					continue;
				}
			}
            // get red_size
			if (egl_conf_attr_sel_.red_size != EGL_DONT_CARE) {
				ret = eglGetConfigAttrib(egl_display, egl_configs[i], EGL_RED_SIZE, &egl_val);
				if (EGL_FALSE == ret) {
                    LOG_E("failed to get EGL red size on config:%d", i);
                    continue;
                }
                else if (egl_val != egl_conf_attr_sel_.red_size) {
					continue;
				}
			}
            // get green size
            if (egl_conf_attr_sel_.green_size != EGL_DONT_CARE) {
                ret = eglGetConfigAttrib(egl_display, egl_configs[i], EGL_GREEN_SIZE, &egl_val);
				if (EGL_FALSE == ret) {
                    LOG_E("failed to get EGL green size on config:%d", i);
                    continue;
                }
                else if (egl_val != egl_conf_attr_sel_.green_size) {
					continue;
				}
			}
            // get blue size
			if (egl_conf_attr_sel_.blue_size != EGL_DONT_CARE) {
                ret = eglGetConfigAttrib(egl_display, egl_configs[i], EGL_BLUE_SIZE, &egl_val);
				if (EGL_FALSE == ret) {
                    LOG_E("failed to get EGL blue size on config:%d", i);
                    continue;
                }
                else if (egl_val != egl_conf_attr_sel_.blue_size) {
					continue;
				}
			}
            // get alpha size
			if (egl_conf_attr_sel_.alpha_size != EGL_DONT_CARE) {
				ret = eglGetConfigAttrib(egl_display, egl_configs[i], EGL_ALPHA_SIZE, &egl_val);
				if (EGL_FALSE == ret) {
                    LOG_E("failed to get EGL alpha size on config:%d", i);
                    continue;
                }
                else if (egl_val != egl_conf_attr_sel_.alpha_size){
					continue;
				}
			}
			// get samples
			if (egl_conf_attr_sel_.samples != EGL_DONT_CARE) {
				ret = eglGetConfigAttrib(egl_display, egl_configs[i], EGL_SAMPLES, &egl_val);
				if (EGL_FALSE == ret) {
					LOG_E("failed to get EGL samples on config:%d", i);
					continue;
				}
				else if (egl_val != egl_conf_attr_sel_.samples) {
					continue;
				}
			}
			// if we get to this point it means that we've matched all our criteria and we can
			// stop searching
			egl_conf_attr_sel_.config_id = egl_configs[i];
			LOG_I("egl configure select:%u success!", egl_conf_attr_sel_.config_id);
			return true;
		}
		return false; 
	}
	bool create_egl_context() {
		// create EGL rendering context
		struct {
			EGLint client_version[2];
			EGLint none;
		} egl_ctx_attr = {
            .client_version = { EGL_CONTEXT_CLIENT_VERSION, 3 },
            .none = EGL_NONE
		};
		egl_context_ = eglCreateContext(egl_disp_, egl_conf_attr_sel_.config_id, EGL_NO_CONTEXT, (EGLint*)&egl_ctx_attr);
		if (egl_context_ == EGL_NO_CONTEXT) {
			LOG_E("EGL context creation failed!");
			return false;
		}
		LOG_I("egl context create success!");
		return true;
	}
	bool make_egl_surface() {
		// create EGL window surface
		egl_surf_ = eglCreateWindowSurface(egl_disp_, egl_conf_attr_sel_.config_id, qnx_screen_display_.get_win(), nullptr);
		if(EGL_NO_SURFACE == egl_surf_) {
			LOG_E("failed to create EGL window surface, error:%d", eglGetError());
			return false;
		}
		// attach EGL rendering context to the surface
		// all OpenGL ES calls will be executed on the context/surface that is currently bind
		// egl_surf is used for both reading and writing
		int ret = eglMakeCurrent(egl_disp_, egl_surf_, egl_surf_, egl_context_);
		if (EGL_FALSE == ret) {
			LOG_E("failed to bind EGL context/surface!");
			return false;
		}
		/* Set swap interval
	 	   This function specifies the minimum number of video frame periods per buffer swap
	       for the window associated with the current context. So, if the interval is 0,
	       the application renders as fast as it can.
	       Interval values of 1 or more limit the rendering to fractions of the display's refresh rate.
	       (For example, 60, 30, 20, 15, etc. frames per second in the case of a display with a refresh rate of 60 Hz.)
	    * */
		ret = eglSwapInterval(egl_disp_, 1);
		if (EGL_FALSE == ret) {
			LOG_E("failed to set Swap interval!");
			return false;
		}
		return true;
	}
	bool create_vertex_shader() {
		// create vertex shader
		vertex_shader_ = glCreateShader(GL_VERTEX_SHADER);
		if (0 == vertex_shader_) {
			LOG_E("failed to create opengl es vertex shader, error:%d", eglGetError());
			return false;
		}
		if (UYVY == image_format_) {
			glShaderSource(vertex_shader_, 1, &uyvy_vertex_shader_source, nullptr);
		}
		glCompileShader(vertex_shader_);
		return compile_is_ok(vertex_shader_);
	}
	bool create_fragment_shader() {
		// create fragment shader
		fragment_shader_ = glCreateShader(GL_FRAGMENT_SHADER);
		if (0 == fragment_shader_) {
			LOG_E("failed to create opengl es fragment shader, error:%d", eglGetError());
			return false;
		}
		if (UYVY == image_format_) {
			glShaderSource(fragment_shader_, 1, &uyvy_fragment_shader_source, nullptr);
		}
		glCompileShader(fragment_shader_);
		return compile_is_ok(fragment_shader_);
	}
	bool create_program() {
		// create program
		program_ = glCreateProgram();
		if (0 == program_) {
			LOG_E("failed to create program!");
			return false;
		}
		glAttachShader(program_, vertex_shader_);
		glAttachShader(program_, fragment_shader_);
		glLinkProgram(program_);
		return program_link_ok(program_);
	}
	void set_vbo_vao() {
		glGenVertexArrays(1, &vao_);
    	glGenBuffers(1, &vbo_);
    	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
 		glBindVertexArray(vao_);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    	glEnableVertexAttribArray(0);
    	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2*sizeof(float)));
  		glEnableVertexAttribArray(1);
		// note that this is allowed, the call to glVertexAttribPointer registered vbo_ as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    	glBindBuffer(GL_ARRAY_BUFFER, 0);
		// you can unbind the vao_ afterwards so other vao_ calls won't accidentally modify this vao_, but this rarely happens. Modifying other
    	// vao_s requires a call to glBindVertexArray anyways so we generally don't unbind vao_s (nor vbo_s) when it's not directly necessary.
    	glBindVertexArray(0);
	}
};