
#define LABEL_COUNT 512


const char *glfw_key_label[LABEL_COUNT];



int input_label_to_key( const char* label ){
	for( int i = 0; i < LABEL_COUNT; i++ ){
		const char* labelb = glfw_key_label[i];
		if(labelb == NULL) continue;
		if( strcmp( label, labelb ) == 0 )
			return i;
	}
	return -1;
}


const char *input_key_to_label(int key){
	return glfw_key_label[key];
}

void command_label_to_key( int argc, char **argv ){
	if(argc != 2) return;
	logf_info("%s = %i", argv[1], input_label_to_key(argv[1]) );
}




void ctx_labels_init(void){
	
	shell_bind_command("label_to_key", &command_label_to_key);


glfw_key_label[GLFW_KEY_SPACE         ] = "space";
glfw_key_label[GLFW_KEY_APOSTROPHE    ] = "apostrophe";
glfw_key_label[GLFW_KEY_COMMA         ] = ",";
glfw_key_label[GLFW_KEY_MINUS         ] = "-";
glfw_key_label[GLFW_KEY_PERIOD        ] = ".";
glfw_key_label[GLFW_KEY_SLASH         ] = "/";
glfw_key_label[GLFW_KEY_0             ] = "0";
glfw_key_label[GLFW_KEY_1             ] = "1";
glfw_key_label[GLFW_KEY_2             ] = "2";
glfw_key_label[GLFW_KEY_3             ] = "3";
glfw_key_label[GLFW_KEY_4             ] = "4";
glfw_key_label[GLFW_KEY_5             ] = "5";
glfw_key_label[GLFW_KEY_6             ] = "6";
glfw_key_label[GLFW_KEY_7             ] = "7";
glfw_key_label[GLFW_KEY_8             ] = "8";
glfw_key_label[GLFW_KEY_9             ] = "9";
glfw_key_label[GLFW_KEY_SEMICOLON     ] = ";";
glfw_key_label[GLFW_KEY_EQUAL         ] = "=";
glfw_key_label[GLFW_KEY_A             ] = "a";
glfw_key_label[GLFW_KEY_B             ] = "b";
glfw_key_label[GLFW_KEY_C             ] = "c";
glfw_key_label[GLFW_KEY_D             ] = "d";
glfw_key_label[GLFW_KEY_E             ] = "e";
glfw_key_label[GLFW_KEY_F             ] = "f";
glfw_key_label[GLFW_KEY_G             ] = "g";
glfw_key_label[GLFW_KEY_H             ] = "h";
glfw_key_label[GLFW_KEY_I             ] = "i";
glfw_key_label[GLFW_KEY_J             ] = "j";
glfw_key_label[GLFW_KEY_K             ] = "k";
glfw_key_label[GLFW_KEY_L             ] = "l";
glfw_key_label[GLFW_KEY_M             ] = "m";
glfw_key_label[GLFW_KEY_N             ] = "n";
glfw_key_label[GLFW_KEY_O             ] = "o";
glfw_key_label[GLFW_KEY_P             ] = "p";
glfw_key_label[GLFW_KEY_Q             ] = "q";
glfw_key_label[GLFW_KEY_R             ] = "r";
glfw_key_label[GLFW_KEY_S             ] = "s";
glfw_key_label[GLFW_KEY_T             ] = "t";
glfw_key_label[GLFW_KEY_U             ] = "u";
glfw_key_label[GLFW_KEY_V             ] = "v";
glfw_key_label[GLFW_KEY_W             ] = "w";
glfw_key_label[GLFW_KEY_X             ] = "x";
glfw_key_label[GLFW_KEY_Y             ] = "y";
glfw_key_label[GLFW_KEY_Z             ] = "z";
glfw_key_label[GLFW_KEY_LEFT_BRACKET  ] = "left_bracket";
glfw_key_label[GLFW_KEY_BACKSLASH     ] = "backslash";
glfw_key_label[GLFW_KEY_RIGHT_BRACKET ] = "right_bracket";
glfw_key_label[GLFW_KEY_GRAVE_ACCENT  ] = "grave_accent";
glfw_key_label[GLFW_KEY_WORLD_1       ] = "world_1";
glfw_key_label[GLFW_KEY_WORLD_2       ] = "world_2";
glfw_key_label[GLFW_KEY_ESCAPE        ] = "escape";
glfw_key_label[GLFW_KEY_ENTER         ] = "enter";
glfw_key_label[GLFW_KEY_TAB           ] = "tab";
glfw_key_label[GLFW_KEY_BACKSPACE     ] = "backspace";
glfw_key_label[GLFW_KEY_INSERT        ] = "insert";
glfw_key_label[GLFW_KEY_DELETE        ] = "delete";
glfw_key_label[GLFW_KEY_RIGHT         ] = "right";
glfw_key_label[GLFW_KEY_LEFT          ] = "left";
glfw_key_label[GLFW_KEY_DOWN          ] = "down";
glfw_key_label[GLFW_KEY_UP            ] = "up";
glfw_key_label[GLFW_KEY_PAGE_UP       ] = "page_up";
glfw_key_label[GLFW_KEY_PAGE_DOWN     ] = "page_down";
glfw_key_label[GLFW_KEY_HOME          ] = "home";
glfw_key_label[GLFW_KEY_END           ] = "end";
glfw_key_label[GLFW_KEY_CAPS_LOCK     ] = "caps_lock";
glfw_key_label[GLFW_KEY_SCROLL_LOCK   ] = "scroll_lock";
glfw_key_label[GLFW_KEY_NUM_LOCK      ] = "num_lock";
glfw_key_label[GLFW_KEY_PRINT_SCREEN  ] = "print_screen";
glfw_key_label[GLFW_KEY_PAUSE         ] = "pause";
glfw_key_label[GLFW_KEY_F1            ] = "f1";
glfw_key_label[GLFW_KEY_F2            ] = "f2";
glfw_key_label[GLFW_KEY_F3            ] = "f3";
glfw_key_label[GLFW_KEY_F4            ] = "f4";
glfw_key_label[GLFW_KEY_F5            ] = "f5";
glfw_key_label[GLFW_KEY_F6            ] = "f6";
glfw_key_label[GLFW_KEY_F7            ] = "f7";
glfw_key_label[GLFW_KEY_F8            ] = "f8";
glfw_key_label[GLFW_KEY_F9            ] = "f9";
glfw_key_label[GLFW_KEY_F10           ] = "f10";
glfw_key_label[GLFW_KEY_F11           ] = "f11";
glfw_key_label[GLFW_KEY_F12           ] = "f12";
glfw_key_label[GLFW_KEY_F13           ] = "f13";
glfw_key_label[GLFW_KEY_F14           ] = "f14";
glfw_key_label[GLFW_KEY_F15           ] = "f15";
glfw_key_label[GLFW_KEY_F16           ] = "f16";
glfw_key_label[GLFW_KEY_F17           ] = "f17";
glfw_key_label[GLFW_KEY_F18           ] = "f18";
glfw_key_label[GLFW_KEY_F19           ] = "f19";
glfw_key_label[GLFW_KEY_F20           ] = "f20";
glfw_key_label[GLFW_KEY_F21           ] = "f21";
glfw_key_label[GLFW_KEY_F22           ] = "f22";
glfw_key_label[GLFW_KEY_F23           ] = "f23";
glfw_key_label[GLFW_KEY_F24           ] = "f24";
glfw_key_label[GLFW_KEY_F25           ] = "f25";
glfw_key_label[GLFW_KEY_KP_0          ] = "kp_0";
glfw_key_label[GLFW_KEY_KP_1          ] = "kp_1";
glfw_key_label[GLFW_KEY_KP_2          ] = "kp_2";
glfw_key_label[GLFW_KEY_KP_3          ] = "kp_3";
glfw_key_label[GLFW_KEY_KP_4          ] = "kp_4";
glfw_key_label[GLFW_KEY_KP_5          ] = "kp_5";
glfw_key_label[GLFW_KEY_KP_6          ] = "kp_6";
glfw_key_label[GLFW_KEY_KP_7          ] = "kp_7";
glfw_key_label[GLFW_KEY_KP_8          ] = "kp_8";
glfw_key_label[GLFW_KEY_KP_9          ] = "kp_9";
glfw_key_label[GLFW_KEY_KP_DECIMAL    ] = "kp_decimal";
glfw_key_label[GLFW_KEY_KP_DIVIDE     ] = "kp_divide";
glfw_key_label[GLFW_KEY_KP_MULTIPLY   ] = "kp_multiply";
glfw_key_label[GLFW_KEY_KP_SUBTRACT   ] = "kp_subtract";
glfw_key_label[GLFW_KEY_KP_ADD        ] = "kp_add";
glfw_key_label[GLFW_KEY_KP_ENTER      ] = "kp_enter";
glfw_key_label[GLFW_KEY_KP_EQUAL      ] = "kp_equal";
glfw_key_label[GLFW_KEY_LEFT_SHIFT    ] = "shift";
glfw_key_label[GLFW_KEY_LEFT_CONTROL  ] = "control";
glfw_key_label[GLFW_KEY_LEFT_ALT      ] = "alt";
glfw_key_label[GLFW_KEY_LEFT_SUPER    ] = "super";
glfw_key_label[GLFW_KEY_RIGHT_SHIFT   ] = "right_shift";
glfw_key_label[GLFW_KEY_RIGHT_CONTROL ] = "right_control";
glfw_key_label[GLFW_KEY_RIGHT_ALT     ] = "right_alt";
glfw_key_label[GLFW_KEY_RIGHT_SUPER   ] = "right_super";
glfw_key_label[GLFW_KEY_MENU          ] = "menu";


}

