//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Computação Gráfica e Visualização I
//               Prof. Eduardo Gastal
//
//     CÓDIGO BASE PARA O TRABALHO FINAL
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Headers abaixo são específicos de C++
#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"
#include "collisions.h"
#include "bezier.h"

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};


// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
void TextRendering_SetColor(float r, float g, float b); // Cor do texto (padrão: preto)
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// ---------------------------------------------------------------------------
// Estado da masmorra / jogo
// ---------------------------------------------------------------------------

// Uma instância é um objeto desenhado na cena: o nome da malha (que fica em
// g_VirtualScene) e a Model matrix que posiciona esta cópia no mundo.
// Várias instâncias compartilham a mesma malha (ex.: todas as paredes).
struct Instance
{
    std::string mesh;
    glm::mat4   model;
    bool        visible = true; // itens coletados deixam de ser desenhados
};

std::vector<Instance>  g_Instances;   // Tudo que é desenhado na masmorra
std::vector<AABB>      g_Colliders;   // Caixas de colisão (paredes, baús)
std::vector<glm::vec3> g_TorchLights; // Posição da luz de cada tocha
glm::vec3 g_StaffPos = glm::vec3(0.0f);   // Onde flutua o cajado mágico (prêmio)

glm::vec3 g_PlayerPos = glm::vec3(0.0f, 0.0f, -3.0f); // Pés do jogador
float g_PlayerYaw     = 0.0f;                         // Rotação em Y
bool  g_PlayerCrouching = false;
bool  g_TopDownCamera = false;                        // Tecla M alterna
bool  g_DungeonMode   = false;                        // false = cena antiga (--lab)

// Câmera em 3ª pessoa: fica atrás do jogador, a uma distância
// g_CameraDistance e elevada pelo ângulo g_CameraPhi. A posição é suavizada
// (g_FollowCameraPos persegue a posição ideal) para não ficar "grudada".
glm::vec3 g_FollowCameraPos = glm::vec3(0.0f);
bool      g_FollowCameraInitialized = false;
const float CAMERA_MIN_DISTANCE = 2.0f;
const float CAMERA_MAX_DISTANCE = 9.0f;
const float CAMERA_MIN_PHI      = 0.05f;  // quase na altura do ombro
const float CAMERA_MAX_PHI      = 1.30f;  // quase vertical
const float CAMERA_SMOOTHING    = 10.0f;  // maior = acompanha mais rápido
const float PLAYER_TURN_SPEED   = 2.2f;   // giro pelas setas (rad/s)
const float MOUSE_TURN_SENSITIVITY = 0.006f;

// Rotação extra da câmera em torno do jogador (0 = atrás dele). A tecla Q
// leva para PI (câmera na frente, vendo o rosto e o que vem atrás); arrastar
// com o botão direito gira livremente. Ao soltar, volta suavemente para 0.
float g_CameraYawOffset       = 0.0f;
float g_CameraYawOffsetManual = 0.0f;
bool  g_LookBack              = false;

// Zoom da câmera de mapa (altura acima do jogador), controlado pela roda, e
// deslocamento do mapa ao arrastar com o botão esquerdo (volta a zero ao
// abrir o mapa de novo).
float g_MapHeight = 30.0f;
glm::vec2 g_MapPan = glm::vec2(0.0f);
const float MAP_MIN_HEIGHT = 12.0f;
const float MAP_MAX_HEIGHT = 80.0f;

// Rotação extra aplicada só ao desenhar o modelo, para que a "frente" do
// modelo .obj coincida com a direção para onde o jogador anda.
const float PLAYER_MODEL_YAW_OFFSET = 0.0f; // o ladrão já olha para +Z

const float PLAYER_SPEED        = 3.5f; // unidades por segundo
const float PLAYER_SPEED_CROUCH = 1.5f;
const float PLAYER_HALF_SIZE    = 0.4f;  // meia largura da caixa de colisão
const float PLAYER_HEIGHT       = 1.6f;
const float PLAYER_MODEL_SCALE  = 0.72f; // o modelo do ladrão tem 2.24 de altura

// O labirinto é uma grade de células de CELL_SIZE x CELL_SIZE (o tamanho do
// piso e do painel de parede do KayKit). Ele é descrito por um "desenho" em
// texto, uma linha por fileira de células (eixo Z) e uma coluna por célula
// (eixo X):
//   '#' parede        '.' chão          'S' início do jogador
//   'c' caixote (dá para se esconder)
//   'b' barril        'B' baú           'K' chave      '$' tesouro
//   'G' portão da sala do tesouro (abre com a chave)
//   'Z' cajado mágico (o prêmio: pegá-lo termina o jogo)
//   'P' pilar decorado
// A sala do tesouro fica no centro (linhas 7..11, colunas 9..13) e tem uma
// decoração diferente: piso de madeira escura, estandartes verdes, ouro e a
// luz verde do cajado.
const float CELL_SIZE = 4.0f;
const char* DUNGEON_MAP[] = {
    "#######################",
    "#S#.......#$.........$#",
    "#.#.#####.###.#######.#",
    "#.#...#$...........K#.#",
    "#.#...#########.#.c.#.#",
    "#.....#..c...c..#...#.#",
    "###.###.#######.#.#.#.#",
    "#b...B#.#$...$#.#...#.#",
    "#.#.###.#.P.P.#.#.###.#",
    "#.#b..#c#..Z..#c#...#.#",
    "#.###.#.#.P.P.#.#.#.#.#",
    "#.....#.#$...$#.#...#.#",
    "#.#####.###G###.#.###.#",
    "#.#$.....c...c..#.....#",
    "#.#.c.###########.c.#.#",
    "#.............#....$..#",
    "#.###########.#.###.#.#",
    "#$.........$#........$#",
    "#######################",
};
const int MAP_ROWS = sizeof(DUNGEON_MAP) / sizeof(DUNGEON_MAP[0]);
int MapCols() { return (int)strlen(DUNGEON_MAP[0]); }
char MapAt(int row, int col)
{
    if (row < 0 || row >= MAP_ROWS || col < 0 || col >= MapCols()) return '#';
    return DUNGEON_MAP[row][col];
}
bool IsTreasureRoom(int row, int col) { return row >= 7 && row <= 11 && col >= 9 && col <= 13; }

// Centro da célula (row, col) no mundo.
glm::vec3 CellCenter(int row, int col) { return glm::vec3(col * CELL_SIZE, 0.0f, row * CELL_SIZE); }

// Névoa: a cor final se mistura com a cor do fundo conforme a distância.
const float FOG_DENSITY = 0.045f;
const glm::vec3 FOG_COLOR(0.02f, 0.02f, 0.03f);

// ---------------------------------------------------------------------------
// Esqueletos
// ---------------------------------------------------------------------------
// Dois tipos:
//  - PATRULHEIRO: anda por uma rota formada por curvas de Bézier cúbicas que
//    passam pelos pontos de ronda (veja bezier.h). A rota pode ser fechada
//    (anda em círculo) ou aberta (vai e volta).
//  - SENTINELA: fica parado num ponto, girando o corpo de um lado para o
//    outro (yaw = base + amplitude * sin(velocidade * t)).
enum SkeletonType { PATROL, SENTINEL };

struct Skeleton
{
    SkeletonType type;
    glm::vec3 pos;
    float yaw;

    // Patrulheiro
    std::vector<CubicBezier> path;
    bool  closed;
    float u;          // posição na rota: parte inteira = trecho, fração = t
    float direction;  // +1 ou -1 (rotas abertas invertem nas pontas)
    float speed;      // unidades por segundo

    // Sentinela (guarda): alterna entre olhar na direção A e na direção B,
    // parando alguns segundos em cada uma.
    float yaw_a, yaw_b, phase;

    // Visão: true se o esqueleto está vendo o jogador neste quadro.
    bool sees_player;
};
std::vector<Skeleton> g_Skeletons;

const float SKELETON_MODEL_SCALE = 0.72f;
const float PATROL_SPEED = 2.4f;
const float GUARD_HOLD_TIME = 2.5f; // segundos olhando para cada lado
const float GUARD_TURN_TIME = 0.8f; // segundos virando

// Visão dos esqueletos: um cone com alcance e meia-abertura. Agachado, o
// jogador só é notado mais de perto; escondido (Espaço), não é notado.
const float VIEW_DISTANCE        = 10.0f;
const float VIEW_DISTANCE_CROUCH = 6.0f;
const float VIEW_HALF_ANGLE      = 0.61f; // 35 graus
const float SKELETON_EYE_HEIGHT  = 1.5f;
const float CATCH_DISTANCE       = 1.1f;  // encostar no esqueleto = captura

// Alerta (0 a 1): sobe enquanto algum esqueleto vê o jogador (mais rápido de
// perto), desce quando ninguém vê. Ao chegar em 1, o jogador é capturado.
float g_Alert = 0.0f;
float g_CaptureTimer = 0.0f; // > 0 durante a "cena" de captura
glm::vec3 g_StartPos;
float g_StartYaw = 0.0f;

// Esconderijos: baús, caixotes e barris. Com Espaço perto de um deles, o
// jogador se esconde (fica agachado, parado e invisível).
struct HideSpot { AABB box; };
std::vector<HideSpot> g_HideSpots;
bool g_PlayerHidden = false;
const float HIDE_DISTANCE = 1.6f;

void UpdateVision(float delta_t);
void CapturePlayer();

// ---------------------------------------------------------------------------
// Objetivo: pegar a chave, abrir o portão e pegar o cajado.
// ---------------------------------------------------------------------------
// Itens coletáveis (a chave e os tesouros): basta passar por cima.
struct Pickup
{
    int       instance;  // índice em g_Instances (para esconder/animar)
    glm::mat4 base_model;
    glm::vec3 pos;
    bool      is_key;
    bool      taken;
};
std::vector<Pickup> g_Pickups;
bool g_HasKey = false;
int  g_TreasureCount = 0, g_TreasureTotal = 0;
const float PICKUP_DISTANCE = 1.3f;

// Portão da sala do tesouro: sobe (animação por delta_t) quando o jogador
// chega perto com a chave; a caixa de colisão é removida.
int       g_GateInstance = -1, g_GateCollider = -1;
glm::mat4 g_GateBaseModel;
glm::vec3 g_GatePos;
AABB      g_GateBox;
float     g_GateOpening = 0.0f; // 0 = fechado, 1 = aberto

// Final: ao pegar o cajado (E), o ladrão dá um pulinho, o cajado solta um
// clarão (a tela vai ficando branca) e o jogo termina.
enum GameState { PLAYING, ENDING, WON };
GameState g_GameState = PLAYING;
float g_EndTimer  = 0.0f;
float g_GameTime  = 0.0f; // tempo de jogo, mostrado no final
bool  g_StaffTaken = false;
const float STAFF_REACH = 2.3f;
const float JUMP_TIME   = 0.6f;
const float FLASH_TIME  = 2.2f; // fim do clarão (tela totalmente branca)

void UpdateObjectives(GLFWwindow* window, float delta_t);
void ResetGame();

// Teclas de ação lidas como evento (no KeyCallback), para nenhum toque
// rápido se perder entre dois quadros.
bool g_InteractPressed = false; // E
bool g_ConfirmPressed  = false; // ENTER

// Distância no plano XZ (ignora a altura).
float FlatDistance(glm::vec3 a, glm::vec3 b)
{
    return glm::length(glm::vec2(a.x - b.x, a.z - b.z));
}

void BuildSkeletons();
void UpdateSkeletons(float delta_t, float time);
void BuildDungeon();
bool PlayerCollides(glm::vec3 feet_position);
bool RayHitsColliders(glm::vec3 origin, glm::vec3 dir, float max_t, float& t_hit);
void ToggleHide();
bool NearHideSpot();

// Malha dinâmica do cone de visão (um "leque" de triângulos no chão,
// recalculado a cada quadro para cada esqueleto).
const int CONE_RAYS = 24;
GLuint g_ConeVAO = 0, g_ConeVBO = 0;
GLint  g_cone_color_uniform;
GLint  g_tint_uniform;
void CreateVisionConeMesh();
void DrawVisionCone(const Skeleton& sk, GLint model_uniform, GLint object_id_uniform);
void UpdatePlayer(GLFWwindow* window, float delta_t);

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLint g_light_positions_uniform;
GLint g_light_colors_uniform;
GLint g_light_count_uniform;
GLint g_ambient_strength_uniform;
GLint g_fog_density_uniform;
GLint g_fog_color_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

int main(int argc, char* argv[])
{
    // A masmorra é a cena padrão. Para ver a cena antiga do laboratório (esfera,
    // coelho e plano), execute com o argumento --lab.
    const bool dungeon_preview =
        !(argc > 1 && std::string(argv[1]) == "--lab");
    g_DungeonMode = dungeon_preview;

    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "Masmorra Stealth 3D".
    GLFWwindow* window;
    window = glfwCreateWindow(800, 600, "Masmorra Stealth 3D", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();

    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/red_brick_diff_1k.jpg");        // TextureImage0
    LoadTextureImage("../../data/rocky_terrain_02_diff_1k.jpg"); // TextureImage1

    if (dungeon_preview)
    {
        LoadTextureImage("../../data/kaykit_dungeon/dungeon_texture.png");    // TextureImage2
        LoadTextureImage("../../data/kaykit_characters/rogue_texture.png");   // TextureImage3
        LoadTextureImage("../../data/kaykit_characters/skeleton_texture.png");// TextureImage4
        LoadTextureImage("../../data/kaykit_characters/mage_texture.png");    // TextureImage5 (cajado)

        // Todas as malhas da masmorra usam a mesma textura (TextureImage2).
        const char* dungeon_models[] = {
            "floor_tile_large", "wall", "chest", "torch_mounted", "box_large",
            "crates_stacked", "barrel_small", "coin_stack_large", "key",
            "wall_gated",
            "floor_wood_large_dark", "pillar_decorated", "banner_patternA_green",
            "banner_thin_green", "chest_gold", "candle_triple", "column",
        };
        for (const char* name : dungeon_models)
        {
            std::string path = std::string("../../data/kaykit_dungeon/") + name + ".obj";
            ObjModel model(path.c_str());
            ComputeNormals(&model);
            BuildTrianglesAndAddToVirtualScene(&model);
        }

        // O ladrão (personagem do jogador).
        ObjModel player_model("../../data/kaykit_characters/rogue.obj");
        ComputeNormals(&player_model);
        BuildTrianglesAndAddToVirtualScene(&player_model);

        // Esqueletos: patrulheiro (capuz) e sentinela (capacete com chifres).
        ObjModel skeleton_model("../../data/kaykit_characters/skeleton.obj");
        ComputeNormals(&skeleton_model);
        BuildTrianglesAndAddToVirtualScene(&skeleton_model);

        ObjModel warrior_model("../../data/kaykit_characters/skeleton_warrior.obj");
        ComputeNormals(&warrior_model);
        BuildTrianglesAndAddToVirtualScene(&warrior_model);

        // O cajado mágico (prêmio da sala do tesouro).
        ObjModel staff_model("../../data/kaykit_characters/staff.obj");
        ComputeNormals(&staff_model);
        BuildTrianglesAndAddToVirtualScene(&staff_model);

        BuildDungeon();
        BuildSkeletons();
        CreateVisionConeMesh();

        g_CameraDistance = 5.5f;
        g_CameraPhi = 0.50f;
    }
    else
    {
        // Construímos a representação de objetos geométricos através de malhas de triângulos
        ObjModel spheremodel("../../data/sphere.obj");
        ComputeNormals(&spheremodel);
        BuildTrianglesAndAddToVirtualScene(&spheremodel);

        ObjModel bunnymodel("../../data/bunny.obj");
        ComputeNormals(&bunnymodel);
        BuildTrianglesAndAddToVirtualScene(&bunnymodel);

        ObjModel planemodel("../../data/plane.obj");
        ComputeNormals(&planemodel);
        BuildTrianglesAndAddToVirtualScene(&planemodel);

        if (argc > 1 && std::string(argv[1]) != "--lab")
        {
            ObjModel model(argv[1]);
            ComputeNormals(&model);
            BuildTrianglesAndAddToVirtualScene(&model);
        }
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();
    if (dungeon_preview)
        TextRendering_SetColor(1.0f, 1.0f, 1.0f); // texto branco sobre fundo escuro

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Instante do quadro anterior, usado para calcular o delta t entre quadros.
    float prev_time = (float)glfwGetTime();

    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        // Tempo decorrido desde o quadro anterior (delta t). Toda movimentação é
        // multiplicada por delta_t, para que a velocidade seja a mesma
        // independentemente da quantidade de quadros por segundo (FPS).
        float current_time = (float)glfwGetTime();
        float delta_t = current_time - prev_time;
        prev_time = current_time;
        if (delta_t > 0.1f) // evita "saltos" caso a janela fique parada por um tempo
            delta_t = 0.1f;

        if (dungeon_preview)
        {
            if (g_GameState == PLAYING)
            {
                UpdatePlayer(window, delta_t);
                UpdateSkeletons(delta_t, current_time);
                UpdateVision(delta_t);
            }
            UpdateObjectives(window, delta_t);
        }

        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        if (dungeon_preview)
        {
            // Fundo escuro de masmorra; no clarão do final, vai ficando branco.
            const float w = (g_GameState == PLAYING) ? 0.0f
                : std::max(0.0f, std::min(1.0f, (g_EndTimer - 0.4f) / (FLASH_TIME - 0.4f)));
            const glm::vec3 bg = FOG_COLOR + (glm::vec3(1.0f, 1.0f, 0.95f) - FOG_COLOR) * w;
            glClearColor(bg.x, bg.y, bg.z, 1.0f);
        }
        else
            glClearColor(0.9f, 0.9f, 1.0f, 1.0f);

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        // Alvo da câmera: na masmorra, o jogador (na altura do tronco); na cena
        // antiga do laboratório, a origem.
        const glm::vec3 camera_target = dungeon_preview
            ? g_PlayerPos + glm::vec3(0.0f, 0.8f, 0.0f)
            : glm::vec3(0.0f, 0.0f, 0.0f);

        glm::vec4 camera_position_c  = glm::vec4(camera_target.x + x, camera_target.y + y, camera_target.z + z, 1.0f); // Ponto "c", centro da câmera
        glm::vec4 camera_lookat_l    = glm::vec4(camera_target, 1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)

        if (dungeon_preview)
        {
            if (g_TopDownCamera)
            {
                // Câmera de mapa: vista de cima, centrada na sala. Como a
                // direção de visão é paralela ao eixo Y, o vetor "up" não pode
                // ser Y; usamos -Z (o "topo" do mapa é o norte da masmorra).
                // Ela acompanha o jogador, para funcionar em todo o labirinto.
                const float map_x = g_PlayerPos.x + g_MapPan.x;
                const float map_z = g_PlayerPos.z + g_MapPan.y;
                camera_lookat_l   = glm::vec4(map_x, 0.0f, map_z, 1.0f);
                camera_position_c = glm::vec4(map_x, g_MapHeight, map_z, 1.0f);
                camera_up_vector  = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
            }
            else
            {
                // Câmera em 3ª pessoa, às costas do jogador.
                //
                // 'forward' é a direção para onde o jogador está virado. A
                // câmera fica no sentido oposto (-forward), elevada pelo
                // ângulo phi: back = -forward*cos(phi) + up*sin(phi).
                const glm::vec3 forward(sin(g_PlayerYaw), 0.0f, cos(g_PlayerYaw));
                const glm::vec3 up(0.0f, 1.0f, 0.0f);

                // O ângulo extra (olhar para trás / girar com botão direito)
                // também é suavizado, para a câmera dar a volta em arco.
                const float offset_target = g_LookBack ? 3.141592f : g_CameraYawOffsetManual;
                g_CameraYawOffset += (offset_target - g_CameraYawOffset) * (1.0f - expf(-8.0f * delta_t));
                const float cam_yaw = g_PlayerYaw + g_CameraYawOffset;
                const glm::vec3 cam_forward(sin(cam_yaw), 0.0f, cos(cam_yaw));
                const glm::vec3 back = -cam_forward*cosf(g_CameraPhi) + up*sinf(g_CameraPhi);

                // Ponto de referência: a cabeça do jogador (mais baixa se agachado).
                const glm::vec3 head = g_PlayerPos + glm::vec3(0.0f, g_PlayerCrouching ? 1.0f : 1.45f, 0.0f);
                const glm::vec3 ideal = head + back * g_CameraDistance;

                // Suavização exponencial: a cada quadro a câmera percorre uma
                // fração da distância até a posição ideal. Usar 1 - e^(-k*dt)
                // faz o resultado não depender do FPS.
                if (!g_FollowCameraInitialized)
                {
                    g_FollowCameraPos = ideal;
                    g_FollowCameraInitialized = true;
                }
                const float alpha = 1.0f - expf(-CAMERA_SMOOTHING * delta_t);
                g_FollowCameraPos += (ideal - g_FollowCameraPos) * alpha;

                // A câmera não pode atravessar paredes: lançamos um raio da
                // cabeça do jogador até a câmera; se ele bate em alguma parede
                // antes, aproximamos a câmera para logo antes da parede.
                glm::vec3 cam = g_FollowCameraPos;
                glm::vec3 to_cam = cam - head;
                const float cam_dist = glm::length(to_cam);
                if (cam_dist > 1e-4f)
                {
                    const glm::vec3 dir = to_cam / cam_dist;
                    float t_hit;
                    if (RayHitsColliders(head, dir, cam_dist, t_hit))
                    {
                        // Sem espaço atrás: aproximamos a câmera e, para não
                        // ficar "dentro" da cabeça do jogador, subimos a
                        // câmera proporcionalmente ao quanto ela foi encurtada.
                        const float d = std::max(0.3f, t_hit - 0.25f);
                        cam = head + dir * d + up * (0.8f * (cam_dist - d));
                    }
                }

                // A câmera olha um pouco à frente do jogador, assim ele fica na
                // parte de baixo da tela e enxergamos o caminho adiante.
                const glm::vec3 look = head + cam_forward * 2.0f;

                camera_position_c = glm::vec4(cam, 1.0f);
                camera_lookat_l   = glm::vec4(look, 1.0f);
            }
        }
        glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada

        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = dungeon_preview ? -150.0f : -10.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        // Luzes pontuais: na masmorra, uma por tocha; na cena antiga, uma só.
        GLfloat light_positions[8][3] = {};
        GLfloat light_colors[8][3]    = {};
        int     light_count           = 0;

        if (dungeon_preview)
        {
            // O shader aceita até 8 luzes; usamos as 8 tochas mais próximas
            // do jogador (as distantes quase não contribuem, por causa da
            // atenuação e da névoa).
            // Candidatas: todas as tochas (laranja, tremulando) e o cajado
            // (verde, pulsando). O índice -1 representa o cajado.
            std::vector<int> order;
            for (size_t i = 0; i < g_TorchLights.size(); ++i) order.push_back((int)i);
            order.push_back(-1);
            auto light_pos = [](int i) {
                if (i >= 0) return g_TorchLights[i];
                return g_StaffTaken ? g_PlayerPos + glm::vec3(0.0f, 3.0f, 0.0f) : g_StaffPos + glm::vec3(0.0f, 1.0f, 0.0f);
            };
            std::sort(order.begin(), order.end(), [&](int a, int b) {
                return glm::length(light_pos(a) - g_PlayerPos) < glm::length(light_pos(b) - g_PlayerPos);
            });

            for (size_t k = 0; k < order.size() && k < 8; ++k)
            {
                const int i = order[k];
                const glm::vec3 lp = light_pos(i);
                glm::vec3 color;
                if (i < 0)
                {
                    // Luz mágica do cajado: verde e intensa, pulsando devagar.
                    const float pulse = 0.8f + 0.2f * sinf(current_time * 2.0f);
                    color = glm::vec3(0.8f, 4.5f, 1.6f) * pulse;
                    // No final, o cajado solta um clarão cada vez mais forte.
                    if (g_GameState != PLAYING)
                        color = glm::vec3(1.0f, 3.0f, 1.5f) * (1.0f + 12.0f * g_EndTimer);
                }
                else
                {
                    // Tremulação do fogo: intensidade varia com o tempo (cada
                    // tocha com uma fase diferente).
                    const float flicker = 0.85f + 0.15f * sinf(current_time*9.0f + i*1.7f) * sinf(current_time*3.7f + i*0.6f);
                    color = glm::vec3(3.2f, 1.5f, 0.55f) * flicker; // cor quente de fogo
                }
                light_positions[k][0] = lp.x; light_positions[k][1] = lp.y; light_positions[k][2] = lp.z;
                light_colors[k][0] = color.x; light_colors[k][1] = color.y; light_colors[k][2] = color.z;
                ++light_count;
            }
        }
        else
        {
            light_positions[0][0] = 2.0f; light_positions[0][1] = 3.0f; light_positions[0][2] = 2.0f;
            light_colors[0][0] = light_colors[0][1] = light_colors[0][2] = 1.0f;
            light_count = 1;
        }

        glUniform3fv(g_light_positions_uniform, 8, &light_positions[0][0]);
        glUniform3fv(g_light_colors_uniform, 8, &light_colors[0][0]);
        glUniform1i(g_light_count_uniform, light_count);
        glUniform1f(g_ambient_strength_uniform, dungeon_preview ? 0.07f : 0.2f);

        // Névoa só na masmorra e fora da câmera de mapa.
        glUniform1f(g_fog_density_uniform, (dungeon_preview && !g_TopDownCamera) ? FOG_DENSITY : 0.0f);
        glUniform3f(g_fog_color_uniform, FOG_COLOR.x, FOG_COLOR.y, FOG_COLOR.z);

        // Na captura, a cena fica avermelhada (mistura com vermelho).
        if (dungeon_preview && g_GameState != PLAYING)
        {
            // Clarão do final: a cena vai ficando branca.
            const float w = std::max(0.0f, std::min(1.0f, (g_EndTimer - 0.4f) / (FLASH_TIME - 0.4f)));
            glUniform4f(g_tint_uniform, 1.0f, 1.0f, 0.95f, w);
        }
        else
            glUniform4f(g_tint_uniform, 0.8f, 0.05f, 0.05f, (dungeon_preview && g_CaptureTimer > 0.0f) ? 0.55f : 0.0f);

        #define SPHERE 0
        #define BUNNY  1
        #define PLANE  2
        #define DUNGEON_OBJECT 3
        #define ROGUE 4
        #define SKELETON 5
        #define STAFF 6

        if (dungeon_preview)
        {
            // Desenhamos todas as instâncias da masmorra. Cada uma usa uma
            // malha já carregada na GPU, mudando apenas a Model matrix.
            glUniform1i(g_object_id_uniform, DUNGEON_OBJECT);
            for (const Instance& inst : g_Instances)
            {
                if (!inst.visible) continue;
                glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(inst.model));
                DrawVirtualObject(inst.mesh.c_str());
            }

            // Desenhamos o jogador. A Model matrix é: levar para a posição do
            // jogador (T) * virar para onde ele anda (R) * reduzir o modelo (S).
            // Ao agachar, achatamos o modelo em Y.
            // Os pés do modelo do ladrão estão em y = 0 (coordenadas do modelo).
            const float s = PLAYER_MODEL_SCALE;
            const float crouch = g_PlayerCrouching ? 0.65f : 1.0f;
            // Pulinho no final: altura = 0.9 * sin(pi * t / duração).
            float jump = 0.0f;
            if (g_GameState != PLAYING && g_EndTimer < JUMP_TIME)
                jump = 0.9f * sinf(3.141592f * g_EndTimer / JUMP_TIME);
            model = Matrix_Translate(g_PlayerPos.x, g_PlayerPos.y + 0.05f + jump, g_PlayerPos.z)
                  * Matrix_Rotate_Y(g_PlayerYaw + PLAYER_MODEL_YAW_OFFSET)
                  * Matrix_Scale(s, s*crouch, s);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, ROGUE);
            DrawVirtualObject("rogue");

            // Esqueletos: mesma ideia (T * R * S). Os patrulheiros "balançam"
            // um pouco para cima e para baixo ao andar.
            glUniform1i(g_object_id_uniform, SKELETON);
            for (const Skeleton& sk : g_Skeletons)
            {
                const float bob = (sk.type == PATROL) ? 0.06f * fabsf(sinf(current_time * 6.0f + sk.phase)) : 0.0f;
                const float ks = SKELETON_MODEL_SCALE;
                model = Matrix_Translate(sk.pos.x, 0.05f + bob, sk.pos.z)
                      * Matrix_Rotate_Y(sk.yaw)
                      * Matrix_Scale(ks, ks, ks);
                glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
                DrawVirtualObject(sk.type == PATROL ? "skeleton" : "skeleton_warrior");
            }

            // Cajado mágico: flutua (sobe e desce) e gira em torno de Y. Depois
            // de pego, fica erguido acima do ladrão.
            if (!g_StaffTaken)
                model = Matrix_Translate(g_StaffPos.x, g_StaffPos.y + 0.15f * sinf(current_time * 1.5f), g_StaffPos.z)
                      * Matrix_Rotate_Y(current_time * 0.8f)
                      * Matrix_Rotate_Z(0.25f)
                      * Matrix_Scale(0.9f, 0.9f, 0.9f);
            else
                model = Matrix_Translate(g_PlayerPos.x, 1.6f + 0.6f * std::min(1.0f, g_EndTimer / JUMP_TIME), g_PlayerPos.z)
                      * Matrix_Rotate_Y(current_time * 3.0f)
                      * Matrix_Scale(0.7f, 0.7f, 0.7f);
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, STAFF);
            DrawVirtualObject("staff");

            // Cones de visão: transparentes, então são desenhados por último,
            // com blending e sem escrever no Z-buffer.
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            for (const Skeleton& sk : g_Skeletons)
                if (glm::length(sk.pos - g_PlayerPos) < 40.0f)
                    DrawVisionCone(sk, g_model_uniform, g_object_id_uniform);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
        else
        {
            // Desenhamos o modelo da esfera
            model = Matrix_Translate(-1.0f,0.0f,0.0f)
                  * Matrix_Rotate_Z(0.6f)
                  * Matrix_Rotate_X(0.2f)
                  * Matrix_Rotate_Y(g_AngleY + (float)glfwGetTime() * 0.1f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, SPHERE);
            DrawVirtualObject("the_sphere");

            // Desenhamos o modelo do coelho
            model = Matrix_Translate(1.0f,0.0f,0.0f)
                  * Matrix_Rotate_X(g_AngleX + (float)glfwGetTime() * 0.1f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, BUNNY);
            DrawVirtualObject("the_bunny");

            // Desenhamos o plano do chão
            model = Matrix_Translate(0.0f,-1.1f,0.0f);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, PLANE);
            DrawVirtualObject("the_plane");
        }

        if (dungeon_preview)
        {
            if (g_ShowInfoText)
            {
                float pad = TextRendering_LineHeight(window);
                TextRendering_PrintString(window,
                    "WASD: move | arrows or left mouse drag: turn | wheel: zoom",
                    -1.0f + pad/10, 1.0f - pad, 1.0f);
                TextRendering_PrintString(window,
                    "C: crouch | Space: hide | Q: look back | M: map (wheel: zoom, drag: pan) | H: hide help",
                    -1.0f + pad/10, 1.0f - 2*pad, 1.0f);
                TextRendering_PrintString(window,
                    g_TopDownCamera ? "Camera: map (top-down)" : "Camera: third person",
                    -1.0f + pad/10, 1.0f - 3*pad, 1.0f);
            }

            // "?" ou "!" sobre os esqueletos que estão vendo o jogador: a
            // posição acima da cabeça é projetada na tela (P * V * ponto, e
            // divisão por w para obter coordenadas normalizadas).
            for (const Skeleton& sk : g_Skeletons)
            {
                if (!sk.sees_player) continue;
                glm::vec4 clip = projection * view * glm::vec4(sk.pos.x, 2.3f, sk.pos.z, 1.0f);
                if (clip.w <= 0.0f) continue;
                const float cw = TextRendering_CharWidth(window);
                if (g_Alert < 0.6f) TextRendering_SetColor(1.0f, 0.9f, 0.2f);
                else                TextRendering_SetColor(1.0f, 0.2f, 0.1f);
                TextRendering_PrintString(window, g_Alert < 0.6f ? "?" : "!",
                                          clip.x/clip.w - cw*1.5f, clip.y/clip.w, 3.0f);
            }

            // Barra de alerta e mensagens.
            {
                const float lh = TextRendering_LineHeight(window);
                char bar[64];
                const int filled = (int)(g_Alert * 20.0f + 0.5f);
                std::string b = "Alert: [";
                for (int i = 0; i < 20; ++i) b += (i < filled) ? '#' : '.';
                b += "]";
                snprintf(bar, sizeof(bar), "%s", b.c_str());
                if (g_Alert > 0.6f) TextRendering_SetColor(1.0f, 0.3f, 0.2f);
                else if (g_Alert > 0.0f) TextRendering_SetColor(1.0f, 0.9f, 0.3f);
                else TextRendering_SetColor(0.7f, 0.7f, 0.7f);
                TextRendering_PrintString(window, bar, -1.0f + lh/10, -1.0f + lh/2, 1.2f);

                // Inventário e tempo (canto inferior direito).
                char inv[96];
                snprintf(inv, sizeof(inv), "Key: %d/1   Treasure: %d/%d   Time: %02d:%02d",
                         g_HasKey ? 1 : 0, g_TreasureCount, g_TreasureTotal,
                         (int)g_GameTime / 60, (int)g_GameTime % 60);
                TextRendering_SetColor(1.0f, 0.85f, 0.4f);
                const float inv_w = strlen(inv) * TextRendering_CharWidth(window) * 1.2f;
                TextRendering_PrintString(window, inv, 1.0f - inv_w - lh/10, -1.0f + lh/2, 1.2f);

                std::string msg;
                if (g_GameState != PLAYING)      msg = "";
                else if (g_CaptureTimer > 0.0f)  msg = "CAUGHT! Back to the start...";
                else if (g_PlayerHidden)         msg = "HIDDEN  (move to leave)";
                else if (!g_StaffTaken && FlatDistance(g_StaffPos, g_PlayerPos) < STAFF_REACH)
                                                 msg = "E: take the magic staff";
                else if (!g_HasKey && FlatDistance(g_GatePos, g_PlayerPos) < 4.5f)
                                                 msg = "The gate is locked. Find the key!";
                else if (NearHideSpot())         msg = "Space: hide";
                if (!msg.empty())
                {
                    TextRendering_SetColor(1.0f, 1.0f, 1.0f);
                    const float w = msg.size() * TextRendering_CharWidth(window) * 1.6f;
                    TextRendering_PrintString(window, msg, -w/2, -0.75f, 1.6f);
                }
                TextRendering_SetColor(1.0f, 1.0f, 1.0f);
            }

            // Tela final (sobre o fundo branco do clarão).
            if (g_GameState == WON)
            {
                const float cw = TextRendering_CharWidth(window);
                auto centered = [&](const std::string& text, float y, float scale) {
                    TextRendering_PrintString(window, text, -(float)text.size() * cw * scale / 2.0f, y, scale);
                };
                char line[96];
                TextRendering_SetColor(0.1f, 0.35f, 0.15f);
                centered("YOU ESCAPED!", 0.25f, 4.0f);
                TextRendering_SetColor(0.15f, 0.15f, 0.15f);
                centered("The magic staff took you back to the real world.", 0.05f, 1.5f);
                snprintf(line, sizeof(line), "Treasure: %d/%d    Time: %02d:%02d",
                         g_TreasureCount, g_TreasureTotal, (int)g_GameTime / 60, (int)g_GameTime % 60);
                centered(line, -0.12f, 1.8f);
                centered("Press ENTER to play again or ESC to quit", -0.35f, 1.3f);
                TextRendering_SetColor(1.0f, 1.0f, 1.0f);
            }
        }
        else
        {
            // Imprimimos na tela os ângulos de Euler que controlam a rotação do
            // terceiro cubo.
            TextRendering_ShowEulerAngles(window);

            // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
            TextRendering_ShowProjection(window);
        }

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");
    g_light_positions_uniform = glGetUniformLocation(g_GpuProgramID, "light_positions");
    g_light_colors_uniform = glGetUniformLocation(g_GpuProgramID, "light_colors");
    g_light_count_uniform = glGetUniformLocation(g_GpuProgramID, "light_count");
    g_ambient_strength_uniform = glGetUniformLocation(g_GpuProgramID, "ambient_strength");
    g_fog_density_uniform = glGetUniformLocation(g_GpuProgramID, "fog_density");
    g_fog_color_uniform   = glGetUniformLocation(g_GpuProgramID, "fog_color");
    g_cone_color_uniform  = glGetUniformLocation(g_GpuProgramID, "cone_color");
    g_tint_uniform        = glGetUniformLocation(g_GpuProgramID, "tint");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage5"), 5);
    glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice e que pertencem ao mesmo "smoothing group".

    // Obtemos a lista dos smoothing groups que existem no objeto
    std::set<unsigned int> sgroup_ids;
    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        assert(model->shapes[shape].mesh.smoothing_group_ids.size() == num_triangles);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);
            unsigned int sgroup = model->shapes[shape].mesh.smoothing_group_ids[triangle];
            assert(sgroup >= 0);
            sgroup_ids.insert(sgroup);
        }
    }

    size_t num_vertices = model->attrib.vertices.size() / 3;
    model->attrib.normals.reserve( 3*num_vertices );

    // Processamos um smoothing group por vez
    for (const unsigned int & sgroup : sgroup_ids)
    {
        std::vector<int> num_triangles_per_vertex(num_vertices, 0);
        std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

        // Acumulamos as normais dos vértices de todos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                glm::vec4  vertices[3];
                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                    const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                    const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                    vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
                }

                const glm::vec4  a = vertices[0];
                const glm::vec4  b = vertices[1];
                const glm::vec4  c = vertices[2];

                const glm::vec4  n = crossproduct(b-a,c-a);

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    num_triangles_per_vertex[idx.vertex_index] += 1;
                    vertex_normals[idx.vertex_index] += n;
                }
            }
        }

        // Computamos a média das normais acumuladas
        std::vector<size_t> normal_indices(num_vertices, 0);

        for (size_t vertex_index = 0; vertex_index < vertex_normals.size(); ++vertex_index)
        {
            if (num_triangles_per_vertex[vertex_index] == 0)
                continue;

            glm::vec4 n = vertex_normals[vertex_index] / (float)num_triangles_per_vertex[vertex_index];
            n /= norm(n);

            model->attrib.normals.push_back( n.x );
            model->attrib.normals.push_back( n.y );
            model->attrib.normals.push_back( n.z );

            size_t normal_index = (model->attrib.normals.size() / 3) - 1;
            normal_indices[vertex_index] = normal_index;
        }

        // Escrevemos os índices das normais para os vértices dos triângulos deste smoothing group
        for (size_t shape = 0; shape < model->shapes.size(); ++shape)
        {
            size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

            for (size_t triangle = 0; triangle < num_triangles; ++triangle)
            {
                unsigned int sgroup_tri = model->shapes[shape].mesh.smoothing_group_ids[triangle];

                if (sgroup_tri != sgroup)
                    continue;

                for (size_t vertex = 0; vertex < 3; ++vertex)
                {
                    tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                    model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index =
                        normal_indices[ idx.vertex_index ];
                }
            }
        }

    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        // Em 2026-05-18, corrigido bug encontrado pelo aluno Arthur Prediger:
        // std::numeric_limits<float>::min() retorna o menor valor positivo
        // normalizado representável, não o menor valor possível (negativo). Para
        // inicializar o limite máximo da bounding box com um valor "muito
        // pequeno", deve ser usado std::numeric_limits<float>::lowest()
        const float minval = std::numeric_limits<float>::lowest();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados 
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
        g_CameraYawOffsetManual = 0.0f; // a câmera volta para trás do jogador
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    if (g_LeftMouseButtonPressed && g_DungeonMode && g_TopDownCamera)
    {
        // Arrastar o mapa: o mundo acompanha o mouse. Um pixel na tela
        // corresponde a 2*h*tan(fov/2)/altura_da_janela unidades no chão
        // (fov = 60 graus). Na câmera de mapa, a direita da tela é +X e o
        // "para baixo" da tela é +Z.
        int width, height;
        glfwGetWindowSize(window, &width, &height);
        const float units_per_pixel = 2.0f * g_MapHeight * tanf(3.141592f / 6.0f) / std::max(height, 1);
        g_MapPan.x -= (float)(xpos - g_LastCursorPosX) * units_per_pixel;
        g_MapPan.y -= (float)(ypos - g_LastCursorPosY) * units_per_pixel;
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
    else if (g_LeftMouseButtonPressed && g_DungeonMode)
    {
        // Na masmorra, arrastar o mouse na horizontal vira o jogador (e a
        // câmera, que está sempre às costas dele); na vertical, sobe/desce a
        // câmera.
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        g_PlayerYaw -= MOUSE_TURN_SENSITIVITY * dx;
        g_CameraPhi += 0.004f * dy;
        g_CameraPhi  = std::max(CAMERA_MIN_PHI, std::min(CAMERA_MAX_PHI, g_CameraPhi));

        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
    else if (g_LeftMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;
    
        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = 3.141592f/2;
        float phimin = -phimax;
    
        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;
    
        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_RightMouseButtonPressed && g_DungeonMode)
    {
        float dx = xpos - g_LastCursorPosX;
        g_CameraYawOffsetManual -= MOUSE_TURN_SENSITIVITY * dx;
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
    else if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_ForearmAngleZ -= 0.01f*dx;
        g_ForearmAngleX += 0.01f*dy;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;
    
        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_TorsoPositionX += 0.01f*dx;
        g_TorsoPositionY -= 0.01f*dy;
    
        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Na câmera de mapa, a roda controla a altura (zoom do mapa).
    if (g_DungeonMode && g_TopDownCamera)
    {
        g_MapHeight = std::max(MAP_MIN_HEIGHT, std::min(MAP_MAX_HEIGHT, g_MapHeight - 3.0f*(float)yoffset));
        return;
    }

    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.3f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;

    // Na masmorra, limitamos o zoom da câmera em 3ª pessoa.
    if (g_DungeonMode)
        g_CameraDistance = std::max(CAMERA_MIN_DISTANCE, std::min(CAMERA_MAX_DISTANCE, g_CameraDistance));
}

void Correcao_KeyCallback(int key, int action, int mod);

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // =======================
    // Não modifique esta chamada! Ela é utilizada para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    Correcao_KeyCallback(key, action, mod);
    // =======================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla espaço, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
        g_InteractPressed = true;
    if ((key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER) && action == GLFW_PRESS)
        g_ConfirmPressed = true;

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && g_DungeonMode)
    {
        ToggleHide();
    }
    else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
        g_ForearmAngleX = 0.0f;
        g_ForearmAngleZ = 0.0f;
        g_TorsoPositionX = 0.0f;
        g_TorsoPositionY = 0.0f;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla M, alternamos entre a câmera em 3ª pessoa
    // e a câmera de mapa (vista de cima).
    if (key == GLFW_KEY_M && action == GLFW_PRESS)
    {
        g_TopDownCamera = !g_TopDownCamera;
        g_MapPan = glm::vec2(0.0f); // o mapa abre centrado no jogador
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);
    
        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :


// Monta a masmorra: uma sala de 12x12 (3x3 pisos de 4x4), cercada por paredes,
// com uma parede divisória, três baús e oito tochas. Note que há apenas UMA
// malha de piso, UMA de parede, UMA de baú e UMA de tocha na GPU: o que muda
// em cada cópia (instância) é só a Model matrix.
void BuildDungeon()
{
    const float PI   = 3.141592f;
    const float PI_2 = 1.570796f;
    const float half = CELL_SIZE / 2.0f;

    // Adiciona uma instância; se 'collidable', também guarda a AABB dela
    // (em coordenadas do mundo) para os testes de colisão.
    auto add = [](const char* mesh, glm::mat4 model, bool collidable)
    {
        Instance inst;
        inst.mesh  = mesh;
        inst.model = model;
        g_Instances.push_back(inst);

        if (collidable)
        {
            const SceneObject& obj = g_VirtualScene[mesh];
            g_Colliders.push_back(AABB_Transform(obj.bbox_min, obj.bbox_max, model));

            // Baús, caixotes e barris também servem de esconderijo.
            const std::string m = mesh;
            if (m.find("box_") == 0 || m.find("crates") == 0 || m.find("barrel") == 0 || m.find("chest") == 0)
                g_HideSpots.push_back({ g_Colliders.back() });
        }
    };

    // Vizinhos de uma célula: deslocamento (linha, coluna) e o ângulo (yaw)
    // que aponta da parede vizinha para dentro da célula atual.
    struct Neighbor { int dr, dc; float yaw_into_cell; };
    const Neighbor neighbors[4] = {
        {-1,  0, 0.0f},   // parede ao norte (-Z): olhar para +Z
        { 1,  0, PI},     // parede ao sul   (+Z): olhar para -Z
        { 0, -1, PI_2},   // parede a oeste  (-X): olhar para +X
        { 0,  1, -PI_2},  // parede a leste  (+X): olhar para -X
    };

    for (int r = 0; r < MAP_ROWS; ++r)
    for (int c = 0; c < MapCols(); ++c)
    {
        const char cell = MapAt(r, c);
        const glm::vec3 center = CellCenter(r, c);

        if (cell == '#')
        {
            // A parede inteira (célula 4x4x4) é uma caixa de colisão. Só
            // guardamos as que encostam em algum chão (as outras nunca são
            // alcançadas).
            bool touches_floor = false;
            for (const Neighbor& nb : neighbors)
                if (MapAt(r + nb.dr, c + nb.dc) != '#') touches_floor = true;
            if (touches_floor)
            {
                AABB box;
                box.min = center + glm::vec3(-half, 0.0f, -half);
                box.max = center + glm::vec3( half, 4.0f,  half);
                g_Colliders.push_back(box);
            }
            continue;
        }

        const bool treasure_room = IsTreasureRoom(r, c);

        // Chão (na sala do tesouro, de madeira escura).
        add(treasure_room ? "floor_wood_large_dark_Cube.159" : "floor_tile_large_Cube.13698",
            Matrix_Translate(center.x, 0.0f, center.z), false);

        // Paredes: para cada vizinho que é parede, um painel (4 de largura,
        // 1 de espessura) na borda entre as duas células, com a espessura
        // dentro da célula da parede. O painel do KayKit está ao longo de X;
        // para as bordas leste/oeste, rotacionamos 90 graus.
        int wall_count = 0;
        for (const Neighbor& nb : neighbors)
        {
            if (MapAt(r + nb.dr, c + nb.dc) != '#') continue;
            ++wall_count;

            const glm::vec3 to_wall((float)nb.dc, 0.0f, (float)nb.dr);
            const glm::vec3 p = center + to_wall * (half + 0.5f);
            const bool along_z = (nb.dc != 0);
            add("wall_Cube.13106", Matrix_Translate(p.x, 0.0f, p.z) * Matrix_Rotate_Y(along_z ? PI_2 : 0.0f), false);

            // Na sala do tesouro, estandartes verdes alternados nas paredes. O
            // modelo do estandarte já fica encostado na face +Z do painel, por
            // isso usamos a mesma posição, virada para dentro da sala.
            if (treasure_room && (r + c) % 2 == 0)
                add((r + c) % 4 == 0 ? "banner_patternA_green_Cube.15678" : "banner_thin_green_Cube.15614",
                    Matrix_Translate(p.x, 0.0f, p.z) * Matrix_Rotate_Y(nb.yaw_into_cell), false);
        }

        // Tochas: em algumas células (padrão fixo, para ficarem espalhadas),
        // presas na primeira parede vizinha. Cada tocha gera uma luz pontual,
        // um pouco à frente e acima dela (onde fica a chama).
        if (wall_count > 0 && !treasure_room && cell != 'G' && (r * 7 + c * 3) % 4 == 0)
        {
            for (const Neighbor& nb : neighbors)
            {
                if (MapAt(r + nb.dr, c + nb.dc) != '#') continue;
                const glm::vec3 to_wall((float)nb.dc, 0.0f, (float)nb.dr);
                const glm::vec3 p = center + to_wall * (half + 0.2f);
                add("torch_mounted_Cylinder.334", Matrix_Translate(p.x, 1.8f, p.z) * Matrix_Rotate_Y(nb.yaw_into_cell), false);
                g_TorchLights.push_back(glm::vec3(p.x, 2.3f, p.z) - to_wall * 0.55f);
                break;
            }
        }

        // Objetos dentro da célula.
        const glm::mat4 at_center = Matrix_Translate(center.x, 0.0f, center.z);
        switch (cell)
        {
        case 'S':
            g_PlayerPos = center;
            // Começa virado para o primeiro vizinho livre.
            for (const Neighbor& nb : neighbors)
                if (MapAt(r + nb.dr, c + nb.dc) != '#') { g_PlayerYaw = nb.yaw_into_cell + PI; break; }
            g_StartPos = g_PlayerPos;
            g_StartYaw = g_PlayerYaw;
            break;
        case 'c':
        {
            // Caixote para se esconder. Num corredor, fica encostado numa
            // parede (para não bloquear a passagem); numa sala, uma caixa e
            // uma pilha de caixotes.
            bool placed = false;
            for (const Neighbor& nb : neighbors)
            {
                if (MapAt(r + nb.dr, c + nb.dc) != '#') continue;
                const glm::vec3 to_wall((float)nb.dc, 0.0f, (float)nb.dr);
                add("box_large_Cube.14519", at_center * Matrix_Translate(to_wall.x*1.2f, 0.0f, to_wall.z*1.2f), true);
                placed = true;
                break;
            }
            if (!placed)
            {
                add("box_large_Cube.14519", at_center * Matrix_Translate(-0.6f, 0.0f, 0.4f), true);
                add("crates_stacked_Cube.023", at_center * Matrix_Translate(0.9f, 0.0f, -0.7f) * Matrix_Rotate_Y(0.4f), true);
            }
            break;
        }
        case 'b':
            add("barrel_small_Cylinder.216", at_center * Matrix_Translate(-0.5f, 0.0f, 0.3f), true);
            add("barrel_small_Cylinder.216", at_center * Matrix_Translate(0.6f, 0.0f, -0.4f), true);
            break;
        case 'B':
            add("chest_Cube.11164", at_center * Matrix_Rotate_Y(0.3f), true);
            break;
        case '$':
            if (treasure_room)
            {
                // Nos cantos da sala do tesouro: baú de ouro e moedas.
                add("chest_gold_Cube.14744", at_center * Matrix_Rotate_Y((r + c) * 0.7f), true);
                add("coin_stack_large_Cube.022", at_center * Matrix_Translate(1.0f, 0.0f, 0.9f) * Matrix_Scale(0.6f, 0.6f, 0.6f), false);
            }
            else
            {
                // Tesouro coletável.
                const glm::mat4 m = at_center * Matrix_Scale(0.7f, 0.7f, 0.7f);
                add("coin_stack_large_Cube.022", m, false);
                g_Pickups.push_back({ (int)g_Instances.size() - 1, m, center, false, false });
                ++g_TreasureTotal;
            }
            break;
        case 'P':
            add("pillar_decorated_Cube.005", at_center, true);
            break;
        case 'Z':
        {
            // Pedestal do cajado, com velas em volta. O cajado em si é
            // desenhado à parte (gira e flutua) e emite uma luz verde.
            add("column_Cube.13695", at_center, true);
            for (int k = 0; k < 4; ++k)
            {
                const float a = PI_2 * k + 0.785f;
                add("candle_triple_Cylinder.308", at_center * Matrix_Translate(1.6f*sinf(a), 0.0f, 1.6f*cosf(a)), false);
            }
            g_StaffPos = center + glm::vec3(0.0f, 1.5f, 0.0f);
            break;
        }
        case 'K':
        {
            // A chave flutua e gira (animada em UpdateObjectives).
            const glm::mat4 m = at_center * Matrix_Translate(0.0f, 1.0f, 0.0f);
            add("key_Torus.035", m, false);
            g_Pickups.push_back({ (int)g_Instances.size() - 1, m, center, true, false });
            break;
        }
        case 'G':
        {
            // Portão da sala do tesouro: grade atravessando o corredor. Se o
            // corredor vai na direção Z (vizinhos norte/sul livres), a grade
            // fica ao longo de X; senão, girada 90 graus. Por enquanto fica
            // sempre fechado (é uma caixa de colisão).
            const bool corridor_along_z = MapAt(r - 1, c) != '#' && MapAt(r + 1, c) != '#';
            g_GateBaseModel = at_center * Matrix_Rotate_Y(corridor_along_z ? 0.0f : PI_2);
            add("wall_gated_Cube.13125", g_GateBaseModel, true);
            g_GateInstance = (int)g_Instances.size() - 1;
            g_GateCollider = (int)g_Colliders.size() - 1;
            g_GateBox      = g_Colliders.back();
            g_GatePos      = center;
            break;
        }
        }
    }
}

// Cria os esqueletos. As rotas são dadas por células do mapa (linha, coluna);
// os centros dessas células são os pontos de passagem da curva de Bézier.
void BuildSkeletons()
{
    struct PatrolDef { bool closed; std::vector<glm::ivec2> cells; };
    const PatrolDef patrols[] = {
        // Dois esqueletos dando a volta no corredor em torno da sala do
        // tesouro (a mesma rota fechada; o segundo começa do lado oposto).
        { true,  { {5, 7}, {5, 15}, {13, 15}, {13, 7} } },
        { true,  { {13, 15}, {13, 7}, {5, 7}, {5, 15} } },
        // Corredores longos (vão e voltam).
        { false, { {15, 7}, {15, 13} } },
        { false, { {3, 8}, {3, 15} } },
        { false, { {2, 21}, {16, 21} } },
    };

    int index = 0;
    for (const PatrolDef& def : patrols)
    {
        // Os cantos da rota são ligados por linhas retas de células, e
        // usamos o centro de CADA célula como ponto de passagem. Com pontos
        // próximos, a curva só arredonda as esquinas dentro do corredor (com
        // pontos muito distantes, as tangentes ficariam enormes e a curva
        // "estufaria" atravessando as paredes).
        std::vector<glm::ivec2> corners = def.cells;
        if (def.closed) corners.push_back(def.cells[0]);
        std::vector<glm::vec3> points;
        points.push_back(CellCenter(corners[0].x, corners[0].y));
        for (size_t k = 1; k < corners.size(); ++k)
        {
            glm::ivec2 cell = corners[k-1];
            const glm::ivec2 step((corners[k].x > cell.x) - (corners[k].x < cell.x),
                                  (corners[k].y > cell.y) - (corners[k].y < cell.y));
            while (cell != corners[k])
            {
                cell += step;
                points.push_back(CellCenter(cell.x, cell.y));
            }
        }
        if (def.closed) points.pop_back(); // o último repetiria o primeiro

        Skeleton sk;
        sk.type      = PATROL;
        sk.path      = BuildBezierPath(points, def.closed);
        sk.closed    = def.closed;
        sk.u         = 0.0f;
        sk.direction = 1.0f;
        sk.speed     = PATROL_SPEED;
        sk.pos       = points[0];
        sk.yaw       = 0.0f;
        sk.yaw_a = sk.yaw_b = 0.0f;
        sk.phase     = 1.3f * index++;
        sk.sees_player = false;
        g_Skeletons.push_back(sk);
    }

    // Guardas: um em cada salinha de chave. Ficam num canto, alternando o
    // olhar entre duas direções. A chave fica no canto oposto, escondida
    // atrás de uma pilha de caixotes: é preciso esperar o guarda olhar para
    // o outro lado e se esconder pelo caminho.
    // yaw: 0 = sul (+Z), PI = norte (-Z), PI/2 = leste (+X), -PI/2 = oeste.
    struct GuardDef { int row, col; float yaw_a, yaw_b; };
    const float PI   = 3.141592f;
    const float PI_2 = 1.570796f;
    const GuardDef guards[] = {
        { 5, 17,  PI,   PI_2 },  // salinha de cima à direita: norte / leste
        { 15, 5,  PI,  -PI_2 },  // salinha de baixo à esquerda: norte / oeste
        { 13, 17, PI_2, 0.0f },  // salinha de baixo à direita: leste / sul
    };
    for (const GuardDef& def : guards)
    {
        Skeleton sk;
        sk.type  = SENTINEL;
        sk.pos   = CellCenter(def.row, def.col);
        sk.yaw_a = def.yaw_a;
        sk.yaw_b = def.yaw_b;
        sk.yaw   = def.yaw_a;
        sk.phase = 1.7f * index++;
        sk.closed = false; sk.u = 0.0f; sk.direction = 1.0f; sk.speed = 0.0f;
        sk.sees_player = false;
        g_Skeletons.push_back(sk);
    }
}

// Gira 'current' em direção a 'target' (ângulos em radianos) pelo caminho
// mais curto, no máximo 'max_step' radianos.
static float TurnTowards(float current, float target, float max_step)
{
    float diff = target - current;
    while (diff >  3.141592f) diff -= 2.0f * 3.141592f;
    while (diff < -3.141592f) diff += 2.0f * 3.141592f;
    if (diff >  max_step) diff =  max_step;
    if (diff < -max_step) diff = -max_step;
    return current + diff;
}

// Atualiza a posição e a direção de todos os esqueletos (proporcional a delta_t).
void UpdateSkeletons(float delta_t, float time)
{
    for (Skeleton& sk : g_Skeletons)
    {
        // Se está vendo o jogador, o esqueleto para e se vira para ele.
        if (sk.sees_player)
        {
            const glm::vec3 to_player = g_PlayerPos - sk.pos;
            sk.yaw = TurnTowards(sk.yaw, atan2f(to_player.x, to_player.z), 3.0f * delta_t);
            continue;
        }

        if (sk.type == SENTINEL)
        {
            // Ciclo: olha para A (parado) -> vira -> olha para B -> vira.
            // Na virada, interpolamos o ângulo com uma curva suave
            // (smoothstep), que começa e termina devagar.
            const float cycle = 2.0f * (GUARD_HOLD_TIME + GUARD_TURN_TIME);
            float k = fmodf(time + sk.phase, cycle);
            float target;
            if (k < GUARD_HOLD_TIME) target = sk.yaw_a;
            else if (k < GUARD_HOLD_TIME + GUARD_TURN_TIME)
            {
                float x = (k - GUARD_HOLD_TIME) / GUARD_TURN_TIME;
                x = x*x*(3.0f - 2.0f*x);
                target = sk.yaw_a + (sk.yaw_b - sk.yaw_a) * x;
            }
            else if (k < 2.0f*GUARD_HOLD_TIME + GUARD_TURN_TIME) target = sk.yaw_b;
            else
            {
                float x = (k - 2.0f*GUARD_HOLD_TIME - GUARD_TURN_TIME) / GUARD_TURN_TIME;
                x = x*x*(3.0f - 2.0f*x);
                target = sk.yaw_b + (sk.yaw_a - sk.yaw_b) * x;
            }
            // Volta suavemente para o ciclo depois de ter olhado o jogador.
            sk.yaw = TurnTowards(sk.yaw, target, 3.0f * delta_t);
            continue;
        }

        const int segments = (int)sk.path.size();

        // Avançamos o parâmetro u de forma que a distância percorrida seja
        // speed * delta_t: como |B'(t)| é a "velocidade" da curva, o passo em
        // t é distância / |B'(t)|. Assim o esqueleto anda com velocidade
        // constante, mesmo nos trechos onde a curva é mais "esticada".
        int   seg = std::min((int)sk.u, segments - 1);
        float t   = sk.u - seg;
        const float curve_speed = std::max(glm::length(sk.path[seg].Tangent(t)), 0.01f);
        sk.u += sk.direction * (sk.speed * delta_t) / curve_speed;

        if (sk.closed)
        {
            // Rota fechada: dá a volta.
            while (sk.u >= segments) sk.u -= segments;
            while (sk.u < 0.0f)      sk.u += segments;
        }
        else
        {
            // Rota aberta: inverte o sentido nas pontas (vai e volta).
            if (sk.u >= segments) { sk.u = 2.0f*segments - sk.u; sk.direction = -1.0f; }
            if (sk.u < 0.0f)      { sk.u = -sk.u;                 sk.direction =  1.0f; }
        }

        seg = std::min((int)sk.u, segments - 1);
        t   = sk.u - seg;
        sk.pos = sk.path[seg].Point(t);

        // O esqueleto olha na direção em que anda (tangente da curva), virando
        // aos poucos para não "estalar" nas inversões.
        const glm::vec3 dir = sk.path[seg].Tangent(t) * sk.direction;
        if (glm::length(dir) > 1e-4f)
            sk.yaw = TurnTowards(sk.yaw, atan2f(dir.x, dir.z), 4.0f * delta_t);
    }
}

// Visão dos esqueletos. Para cada esqueleto, o jogador é visto se:
//  1) está a menos de VIEW_DISTANCE (menos ainda se agachado);
//  2) está dentro do cone: o ângulo entre a direção do olhar e a direção
//     até o jogador é menor que VIEW_HALF_ANGLE (comparamos cossenos, via
//     produto escalar de vetores normalizados);
//  3) nada bloqueia a linha de visão: lançamos um raio do olho do esqueleto
//     até a cabeça do jogador e testamos contra todas as caixas de colisão
//     (paredes, caixotes, baús...). Agachado, a cabeça fica mais baixa e o
//     raio bate nos caixotes.
// O alerta sobe enquanto algum esqueleto vê o jogador.
void UpdateVision(float delta_t)
{
    bool seen = false;
    float closest = 1e9f;
    const float head_height = g_PlayerCrouching || g_PlayerHidden ? 0.8f : 1.4f;
    const glm::vec3 head = g_PlayerPos + glm::vec3(0.0f, head_height, 0.0f);
    const float range = g_PlayerCrouching ? VIEW_DISTANCE_CROUCH : VIEW_DISTANCE;

    for (Skeleton& sk : g_Skeletons)
    {
        sk.sees_player = false;
        if (g_PlayerHidden || g_CaptureTimer > 0.0f)
            continue;

        const glm::vec3 eye = sk.pos + glm::vec3(0.0f, SKELETON_EYE_HEIGHT, 0.0f);
        glm::vec3 to_player = head - eye;
        const float dist = glm::length(to_player);

        // Encostar no esqueleto: captura imediata.
        const glm::vec3 flat = g_PlayerPos - sk.pos;
        if (glm::length(glm::vec3(flat.x, 0.0f, flat.z)) < CATCH_DISTANCE)
        {
            g_Alert = 1.0f;
            sk.sees_player = true;
            continue;
        }

        if (dist > range) continue;

        const glm::vec3 look(sinf(sk.yaw), 0.0f, cosf(sk.yaw));
        const glm::vec3 dir_flat = glm::normalize(glm::vec3(to_player.x, 0.0f, to_player.z));
        if (glm::dot(look, dir_flat) < cosf(VIEW_HALF_ANGLE)) continue;

        float t_hit;
        if (RayHitsColliders(eye, to_player / dist, dist, t_hit)) continue;

        sk.sees_player = true;
        seen = true;
        closest = std::min(closest, dist);
    }

    if (seen)
        g_Alert += delta_t * (0.35f + 3.0f / (closest + 1.0f));
    else
        g_Alert -= delta_t * 0.5f;
    g_Alert = std::max(0.0f, std::min(1.0f, g_Alert));

    if (g_Alert >= 1.0f && g_CaptureTimer <= 0.0f)
        CapturePlayer();
}

// Captura: começa a "cena" de captura (tela avermelhada por alguns
// segundos); ao final, o jogador volta ao início (veja UpdatePlayer).
void CapturePlayer()
{
    g_CaptureTimer = 2.0f;
    g_PlayerHidden = false;
}

// Verdadeiro se o jogador está perto de algum esconderijo (distância, no
// plano XZ, do jogador até a caixa do objeto).
bool NearHideSpot()
{
    for (const HideSpot& h : g_HideSpots)
    {
        const float dx = std::max(std::max(h.box.min.x - g_PlayerPos.x, 0.0f), g_PlayerPos.x - h.box.max.x);
        const float dz = std::max(std::max(h.box.min.z - g_PlayerPos.z, 0.0f), g_PlayerPos.z - h.box.max.z);
        if (sqrtf(dx*dx + dz*dz) < HIDE_DISTANCE)
            return true;
    }
    return false;
}

// Tenta esconder (ou tirar do esconderijo) o jogador.
void ToggleHide()
{
    if (g_PlayerHidden) { g_PlayerHidden = false; return; }
    if (g_CaptureTimer <= 0.0f && NearHideSpot())
        g_PlayerHidden = true;
}

// Cria o VAO/VBO do cone de visão. Os vértices são atualizados a cada
// desenho (GL_DYNAMIC_DRAW): o ápice no esqueleto e CONE_RAYS+1 pontos no
// arco, formando um "leque" (GL_TRIANGLE_FAN). Só usamos a posição
// (location 0); normais e coordenadas de textura não são necessárias.
void CreateVisionConeMesh()
{
    glGenVertexArrays(1, &g_ConeVAO);
    glBindVertexArray(g_ConeVAO);
    glGenBuffers(1, &g_ConeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, g_ConeVBO);
    glBufferData(GL_ARRAY_BUFFER, (CONE_RAYS + 2) * 4 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

// Desenha o cone de visão de um esqueleto no chão. Cada um dos raios do
// leque é "cortado" onde bate numa parede ou objeto (raio x AABB), então o
// cone mostra exatamente a área que o esqueleto enxerga, com as "sombras"
// atrás dos caixotes, onde dá para se esconder.
void DrawVisionCone(const Skeleton& sk, GLint model_uniform, GLint object_id_uniform)
{
    #define VISION_CONE 7
    const float y_draw = 0.08f;  // um pouco acima do piso
    const float y_test = 1.0f;   // altura dos raios para o corte
    std::vector<float> v;
    v.reserve((CONE_RAYS + 2) * 4);
    v.insert(v.end(), { sk.pos.x, y_draw, sk.pos.z, 1.0f });
    for (int i = 0; i <= CONE_RAYS; ++i)
    {
        const float a = sk.yaw - VIEW_HALF_ANGLE + 2.0f * VIEW_HALF_ANGLE * i / CONE_RAYS;
        const glm::vec3 dir(sinf(a), 0.0f, cosf(a));
        float t = VIEW_DISTANCE;
        float t_hit;
        if (RayHitsColliders(glm::vec3(sk.pos.x, y_test, sk.pos.z), dir, VIEW_DISTANCE, t_hit))
            t = t_hit;
        v.insert(v.end(), { sk.pos.x + dir.x * t, y_draw, sk.pos.z + dir.z * t, 1.0f });
    }

    glBindBuffer(GL_ARRAY_BUFFER, g_ConeVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, v.size() * sizeof(float), v.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Amarelo quando está só olhando; vermelho quando vê o jogador.
    if (sk.sees_player) glUniform4f(g_cone_color_uniform, 1.0f, 0.15f, 0.05f, 0.40f);
    else                glUniform4f(g_cone_color_uniform, 1.0f, 0.85f, 0.25f, 0.22f);

    const glm::mat4 identity = Matrix_Identity();
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(identity));
    glUniform1i(object_id_uniform, VISION_CONE);
    glBindVertexArray(g_ConeVAO);
    glDrawArrays(GL_TRIANGLE_FAN, 0, CONE_RAYS + 2);
    glBindVertexArray(0);
}

// Coleta de itens, portão e final do jogo.
void UpdateObjectives(GLFWwindow* window, float delta_t)
{
    const float time = (float)glfwGetTime();

    if (g_GameState == PLAYING)
    {
        g_GameTime += delta_t;

        // Itens: a chave gira e flutua; passar por cima coleta.
        for (Pickup& p : g_Pickups)
        {
            if (p.taken) continue;
            if (p.is_key)
                g_Instances[p.instance].model = p.base_model
                    * Matrix_Translate(0.0f, 0.15f * sinf(time * 2.5f), 0.0f)
                    * Matrix_Rotate_Y(time * 2.0f)
                    * Matrix_Translate(-0.25f, 0.0f, 0.0f) // centro da argola
                    * Matrix_Rotate_Z(1.570796f);
            if (FlatDistance(p.pos, g_PlayerPos) < PICKUP_DISTANCE && g_CaptureTimer <= 0.0f)
            {
                p.taken = true;
                g_Instances[p.instance].visible = false;
                if (p.is_key) g_HasKey = true;
                else          ++g_TreasureCount;
            }
        }

        // Portão: com a chave, abre ao chegar perto.
        if (g_HasKey && g_GateOpening == 0.0f && FlatDistance(g_GatePos, g_PlayerPos) < 4.5f)
            g_GateOpening = 0.0001f;

        // E perto do cajado: pegar.
        if (!g_StaffTaken && FlatDistance(g_StaffPos, g_PlayerPos) < STAFF_REACH && g_InteractPressed)
        {
            g_StaffTaken = true;
            g_GameState  = ENDING;
            g_EndTimer   = 0.0f;
            g_PlayerHidden = false;
            g_Alert = 0.0f;
            for (Skeleton& sk : g_Skeletons) sk.sees_player = false;
        }
    }
    else
    {
        g_EndTimer += delta_t;
        if (g_GameState == ENDING && g_EndTimer >= FLASH_TIME + 0.4f)
            g_GameState = WON;
        if (g_GameState == WON && g_ConfirmPressed)
            ResetGame();
    }
    g_InteractPressed = false;
    g_ConfirmPressed  = false;

    // Animação do portão subindo (1.5 s). Quando já subiu o bastante, a
    // caixa de colisão é levada para longe (o jogador passa).
    if (g_GateOpening > 0.0f && g_GateOpening < 1.0f)
    {
        g_GateOpening = std::min(1.0f, g_GateOpening + delta_t / 1.5f);
        const float s = g_GateOpening * g_GateOpening * (3.0f - 2.0f * g_GateOpening);
        g_Instances[g_GateInstance].model = Matrix_Translate(0.0f, 3.7f * s, 0.0f) * g_GateBaseModel;
        if (g_GateOpening > 0.6f)
        {
            g_Colliders[g_GateCollider].min = glm::vec3(0.0f, -200.0f, 0.0f);
            g_Colliders[g_GateCollider].max = glm::vec3(0.0f, -199.0f, 0.0f);
        }
    }
}

// Recomeça o jogo do zero (ENTER na tela final).
void ResetGame()
{
    g_GameState = PLAYING;
    g_EndTimer = 0.0f;
    g_GameTime = 0.0f;
    g_StaffTaken = false;
    g_HasKey = false;
    g_TreasureCount = 0;
    for (Pickup& p : g_Pickups)
    {
        p.taken = false;
        g_Instances[p.instance].visible = true;
        g_Instances[p.instance].model = p.base_model;
    }
    g_GateOpening = 0.0f;
    g_Instances[g_GateInstance].model = g_GateBaseModel;
    g_Colliders[g_GateCollider] = g_GateBox;
    g_PlayerPos = g_StartPos;
    g_PlayerYaw = g_StartYaw;
    g_PlayerHidden = false;
    g_Alert = 0.0f;
    g_CaptureTimer = 0.0f;
    g_FollowCameraInitialized = false;
}

// Teste de colisão do jogador: monta a AABB do jogador (centrada em 'p', com os
// pés em p.y) e testa contra todas as AABBs de paredes e baús.
bool PlayerCollides(glm::vec3 p)
{
    AABB player;
    player.min = glm::vec3(p.x - PLAYER_HALF_SIZE, p.y,                p.z - PLAYER_HALF_SIZE);
    player.max = glm::vec3(p.x + PLAYER_HALF_SIZE, p.y + PLAYER_HEIGHT, p.z + PLAYER_HALF_SIZE);

    for (const AABB& box : g_Colliders)
        if (AABB_Intersects(player, box))
            return true;

    return false;
}

// Testa um raio contra todas as caixas de colisão da masmorra. Devolve true se
// ele atinge alguma antes da distância max_t; t_hit recebe a mais próxima.
bool RayHitsColliders(glm::vec3 origin, glm::vec3 dir, float max_t, float& t_hit)
{
    bool hit = false;
    t_hit = max_t;
    for (const AABB& box : g_Colliders)
    {
        float t;
        if (Ray_AABB(origin, dir, box, t) && t < t_hit)
        {
            t_hit = t;
            hit = true;
        }
    }
    return hit;
}

// Lê o teclado e move o jogador. Controles "de jogo em 3ª pessoa": o jogador
// tem uma direção (g_PlayerYaw); W/S andam para frente/trás nessa direção,
// A/D andam de lado, e as setas (ou o mouse) giram o jogador. Tudo é
// proporcional a delta_t.
void UpdatePlayer(GLFWwindow* window, float delta_t)
{
    // Durante a captura o jogador não se move; ao terminar, volta ao início.
    if (g_CaptureTimer > 0.0f)
    {
        g_CaptureTimer -= delta_t;
        if (g_CaptureTimer <= 0.0f)
        {
            g_PlayerPos = g_StartPos;
            g_PlayerYaw = g_StartYaw;
            g_Alert = 0.0f;
            g_FollowCameraInitialized = false;
        }
        return;
    }

    // Escondido: parado e agachado. Andar sai do esconderijo.
    if (g_PlayerHidden)
    {
        g_PlayerCrouching = true;
        g_LookBack = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            g_PlayerHidden = false;
        else
            return;
    }

    if (glfwGetKey(window, GLFW_KEY_LEFT)  == GLFW_PRESS) g_PlayerYaw += PLAYER_TURN_SPEED * delta_t;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) g_PlayerYaw -= PLAYER_TURN_SPEED * delta_t;

    // "Frente" do jogador e "direita" (frente x up), no plano XZ.
    const glm::vec3 forward(sin(g_PlayerYaw), 0.0f, cos(g_PlayerYaw));
    const glm::vec3 right  (-cos(g_PlayerYaw), 0.0f, sin(g_PlayerYaw));

    glm::vec3 direction(0.0f);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) direction += forward;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) direction -= forward;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) direction += right;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) direction -= right;

    g_PlayerCrouching = (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS);
    g_LookBack        = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS);

    if (glm::length(direction) < 1e-5f)
        return;

    direction = glm::normalize(direction);

    const float speed = g_PlayerCrouching ? PLAYER_SPEED_CROUCH : PLAYER_SPEED;
    const glm::vec3 step = direction * speed * delta_t;

    // Movemos um eixo de cada vez. Assim, ao bater numa parede na diagonal, o
    // jogador "desliza" ao longo dela em vez de ficar preso.
    glm::vec3 candidate = g_PlayerPos;
    candidate.x += step.x;
    if (!PlayerCollides(candidate))
        g_PlayerPos = candidate;

    candidate = g_PlayerPos;
    candidate.z += step.z;
    if (!PlayerCollides(candidate))
        g_PlayerPos = candidate;
}
