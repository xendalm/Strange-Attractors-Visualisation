#include <gl\glew.h>
#include <gl\glut.h>
#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <time.h>
#include <cmath>
using namespace std;

string loadFile(string fileName) {
	ifstream file(fileName);
	string s;
	getline(file, s, '\0');
	file.close();
	return s;
}
GLuint compileShaders(string fileName, GLenum type) {
	string s = loadFile(fileName);
	const char* shaderCode = s.c_str();
	GLuint shaderId = glCreateShader(type);
	glShaderSource(shaderId, 1, &shaderCode, NULL);
	glCompileShader(shaderId);
	GLint compileStatus;
	glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);
	if (!compileStatus) {
		int length;
		glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &length);
		char* cMessage = new char[length];
		glGetShaderInfoLog(shaderId, length, &length, cMessage);
		std::cout << "Cannot Compile Shader: " << cMessage;
		delete[] cMessage;
		glDeleteShader(shaderId);
		return 0;
	}
	return shaderId;
}

struct RGB {
    float R, G, B;
};
float HueToRGB(float v1, float v2, float vH) {
    if (vH < 0) vH += 1;
    if (vH > 1) vH -= 1;
    if ((6 * vH) < 1) return (v1 + (v2 - v1) * 6 * vH);
    if ((2 * vH) < 1) return v2;
    if ((3 * vH) < 2) return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);
    return v1;
}
RGB HSLToRGB(int h, float s, float l) {
    float r = 0;
    float g = 0;
    float b = 0;
    if (s == 0)
        r = g = b = l;
    else {
        float v1, v2;
        float hue = (float)h / 360;
        v2 = (l < 0.5) ? (l * (1 + s)) : ((l + s) - (l * s));
        v1 = 2 * l - v2;
        r = HueToRGB(v1, v2, hue + (1.0f / 3));
        g = HueToRGB(v1, v2, hue);
        b = HueToRGB(v1, v2, hue - (1.0f / 3));
    }
    return { r, g, b };
}

struct point_3d {
    float x, y, z;
};

float rotate_x = 20;
float rotate_z = -20;

class Visualisation {
private:
    vector<point_3d> vertex;
    GLuint vertexVBO;
    vector<RGB> color;
    GLuint colorVBO;
    vector<GLuint> index;
    GLuint indexEBO;
    unsigned int iterations = 0;
    unsigned int cur_index = 0; 
    int step;

    float axis_shift = 0.6;
    GLfloat scale[3] = {1, 1, 1};
    GLfloat translate[3] = {0, 0, 0};
public:
    void init(const vector<point_3d>& points, int _step = -1, bool set_scale_for_each_axis = false) {
        if (points.size() < 1) {
            cout << "Empty vector";
            exit(EXIT_FAILURE);
        }
        iterations = points.size();
        vertex = points;
        color.resize(iterations);
        index.resize(iterations);
        step = _step;

        float min_x, max_x;
        float min_y, max_y;
        float min_z, max_z;
        min_x = max_x = vertex[0].x;
        min_y = max_y = vertex[0].y;
        min_z = max_z = vertex[0].z;

        for (int i = 0; i < vertex.size(); i++) {
            min_x = min(min_x, vertex[i].x);
            max_x = max(max_x, vertex[i].x);
            min_y = min(min_y, vertex[i].y);
            max_y = max(max_y, vertex[i].y);
            min_z = min(min_z, vertex[i].z);
            max_z = max(max_z, vertex[i].z);
            //round(double(i) / iterations * 360)

            color[i] = HSLToRGB(i % 36, 0.8, 0.5);
            index[i] = i;
        }
        /*cout << min_x << " " << max_x << endl;
         cout << min_y << " " << max_y << endl;
         cout << min_z << " " << max_z << endl;*/

        // scale
        if (set_scale_for_each_axis) {
            scale[0] = float(1) / (abs(max_x - min_x) / 2) * axis_shift;
            scale[1] = float(1) / (abs(max_y - min_y) / 2) * axis_shift;
            scale[2] = float(1) / (abs(max_z - min_z) / 2) * axis_shift;
        }
        else {
            float k = float(1) / max(max((abs(max_x - min_x) / 2), abs(max_y - min_y) / 2), abs(max_z - min_z) / 2);
            for (GLfloat& i : scale) i = k * axis_shift;
        }

        // shift
        /*cout << (min_x + max_x) / 2 << endl;
        cout << (min_y + max_y) / 2 << endl;
        cout << (min_z + max_z) / 2 << endl;*/
        translate[0] = -(min_x + max_x) / 2;
        translate[1] = -(min_y + max_y) / 2;
        translate[2] = -(min_z + max_z) / 2;

        glGenBuffers(1, &vertexVBO);
        glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
        glBufferData(GL_ARRAY_BUFFER, vertex.size() * sizeof(vertex[0]), vertex.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glGenBuffers(1, &colorVBO);
        glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
        glBufferData(GL_ARRAY_BUFFER, color.size() * sizeof(color[0]), color.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glGenBuffers(1, &indexEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index.size() * sizeof(index[0]), index.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    ~Visualisation() {
        glDeleteBuffers(1, &vertexVBO);
        glDeleteBuffers(1, &colorVBO);
        glDeleteBuffers(1, &indexEBO);
    }
    void next_fame() {
        if (step == -1){
            cur_index = iterations;
            return;
        }
        cur_index += step;
        if (cur_index > iterations) cur_index = 100;
    }

    void show() {
        glPushMatrix();
        glDepthMask(false);

        glScalef(scale[0], scale[1], scale[2]);
        glTranslatef(translate[0], translate[1], translate[2]);

        glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
        glVertexPointer(3, GL_FLOAT, 0, NULL);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
        glColorPointer(3, GL_FLOAT, 0, NULL);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_COLOR_ARRAY);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexEBO);
            glDrawElements(GL_LINE_STRIP, cur_index, GL_UNSIGNED_INT, NULL);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

        glDepthMask(true);
        glPopMatrix();
    }
} V;

void draw_string(void* font, const char* string) {
    while (*string)
        glutStrokeCharacter(font, *string++);
}

void show_axis(float alpha, float beta, const char* label) {
    float arrow_size = 0.05;
    glPushMatrix();
    glRotatef(beta, 1, 0, 0);
    glRotatef(alpha, 0, 0, 1);
    glBegin(GL_LINES);
    glVertex2f(-1, 0);
    glVertex2f(1, 0);
    glEnd();

    glPushMatrix();
    if (beta > 0) glRotatef(alpha - rotate_z, 1, 0, 0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(1 - arrow_size, 0 - arrow_size / 2);
    glVertex2f(1, 0);
    glVertex2f(1 - arrow_size, 0 + arrow_size / 2);
    glEnd();
    glPopMatrix();

    glColor3f(1, 1, 1);
    if (beta > 0) glTranslatef(1 + arrow_size, 0, 0);
    else  glTranslatef(1, 0, arrow_size);
    glRotatef(90 + beta, 0, 1, 0);
    glRotatef(alpha + rotate_z, 1, 0, 0);
    glRotatef(90, 0, 0, 1);
    glScalef(0.0005, 0.0005, 0.0005);
    draw_string(GLUT_STROKE_ROMAN, label);

    glPopMatrix();
}

void display() {
    glClearDepth(0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glRotatef(rotate_x, 1.0, 0.0, 0.0);
    glRotatef(rotate_z, 0.0, 1.0, 0.0);
    glRotatef(-90, 1.0, 0.0, 0.0);
    glRotatef(-90, 0.0, 0.0, 1.0);
    glScalef(0.7, 0.7, 0.7);

    glLineWidth(2);
    glColor3d(0.5, 0.5, 0.5);
    show_axis(0, 0, "x");
    glColor3d(0.5, 0.5, 0.5);
    show_axis(90, 0, "y");
    glColor3d(0.5, 0.5, 0.5);
    show_axis(90, 90, "z");

    V.show();

    glutSwapBuffers();
}

void timer(int) {
    rotate_z += 0.25;
    V.next_fame();
    glutPostRedisplay();
    glutTimerFunc(1000 / 60, timer, 0);
}

void specialKeys(int key, int x, int y) {
    if (key == GLUT_KEY_RIGHT)
        rotate_z += 5;
    else if (key == GLUT_KEY_LEFT)
        rotate_z -= 5;
    else if (key == GLUT_KEY_UP)
        rotate_x += 5;
    else if (key == GLUT_KEY_DOWN)
        rotate_x -= 5;
}

vector <point_3d> get_Lorenz_attractor(int iterations) {
    float x = 0.01, y = 0, z = 0;
    float sigma = 10, rho = 28, beta = float(8) / 3;
    float dt = 0.01;
    vector <point_3d> result = { {x, y, z} };
    for (int i = 0; i < iterations; i++) {
        float dx = (sigma * (y - x)) * dt;
        float dy = (x * (rho - z) - y) * dt;
        float dz = (x * y - beta * z) * dt;
        x += dx;
        y += dy;
        z += dz;
        result.push_back({ x, y, z });
    }
    return result;
}

vector <point_3d> get_Lorenz_attractor_mod_2(int iterations) {
    float x = 0.1, y = 0.1, z = 0.1;
    float a = 0.9, b = 5.0, c = 9.9, d = 1.0, dt = 0.003;
    vector <point_3d> result = { {x, y, z} };
    for (int i = 0; i < iterations; i++) {
        float dx = (-a * x + y * y - z * z + a * c) * dt;
        float dy = (x * (y - b * z) + d) * dt;
        float dz = (-z + x * (b * y + z)) * dt;
        x += dx;
        y += dy;
        z += dz;
        result.push_back({ x, y, z });
    }
    return result;
}

vector <point_3d> get_Aizawa_attractor(int iterations) {
    float x = 0.1, y = 0, z = 0;
    float a = 0.95, b = 0.7, c = 0.6, d = 3.5, e = 0.25, f = 0.1;
    float dt = 0.01;
    vector <point_3d> result = { {x, y, z} };
    for (int i = 0; i < iterations; i++) {
        float dx = (z - b) * x - d * y;
        float dy = d * x + (z - b) * y;
        float dz = c + a * z - (pow(z, 3) / 3) - (pow(x, 2) + pow(y, 2)) * (1 + e * z) + f * z * (pow(x, 3));
        x += dx * dt;
        y += dy * dt;
        z += dz * dt;
        result.push_back({ x, y, z });
    }
    return result;
}


void initShaders() {
    GLuint vShaderId = compileShaders("vertexShader.vert", GL_VERTEX_SHADER);
    GLuint fShaderId = compileShaders("fragmentShader.frag", GL_FRAGMENT_SHADER);
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vShaderId);
    glAttachShader(programId, fShaderId);
    glLinkProgram(programId);
    glUseProgram(programId);
}
void initGL() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);

    glDepthFunc(GL_GREATER);
    glEnable(GL_DEPTH_TEST);
}

int main(int argc, char* argv[]) {
    glutInit(&argc, argv); 
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(400, 400);
    glutInitWindowSize(700, 700);
    glutCreateWindow("Visualization");
    glutSpecialFunc(specialKeys);
    glutTimerFunc(500, timer, 0);
    glewInit();
    initGL();
    //initShaders();

    V.init(get_Aizawa_attractor(50000), 5);

    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}

