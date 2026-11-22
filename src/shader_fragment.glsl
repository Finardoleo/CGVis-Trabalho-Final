#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da posição global e a normal de cada vértice, definidas em
// "shader_vertex.glsl" e "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define SPHERE 0
#define BUNNY  1
#define PLANE  2
#define DUNGEON_OBJECT 3
#define ROGUE 4
#define SKELETON 5
#define STAFF 6
#define VISION_CONE 7
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3; // textura do ladrão
uniform sampler2D TextureImage4; // textura dos esqueletos
uniform sampler2D TextureImage5; // textura do cajado
uniform vec3 light_positions[8];
uniform vec3 light_colors[8];
uniform int light_count;
uniform float ambient_strength;

// Material: intensidade (Ks) e expoente (q) do termo especular de Phong.
uniform float specular_strength;
uniform float shininess;

// Névoa exponencial: densidade (0 = desligada) e cor (a mesma do fundo).
uniform float fog_density;
uniform vec3  fog_color;

// Cor (com transparência) do cone de visão dos esqueletos.
uniform vec4 cone_color;

// Tingimento da cena inteira (rgb = cor, a = quanto misturar). Usado para
// deixar a tela avermelhada quando o jogador é capturado.
uniform vec4 tint;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

void main()
{
    // O cone de visão não tem iluminação: só uma cor semitransparente.
    if ( object_id == VISION_CONE )
    {
        color = cone_color;
        return;
    }

    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

	// Coeficiente de refletância difusa
	vec3 Kd0;

    if ( object_id == SPHERE )
    {
        // PREENCHA AQUI as coordenadas de textura da esfera, computadas com
        // projeção esférica EM COORDENADAS DO MODELO. Utilize como referência
        // o slides 134-150 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // A esfera que define a projeção deve estar centrada na posição
        // "bbox_center" definida abaixo.

        // Você deve utilizar:
        //   função 'length( )' : comprimento Euclidiano de um vetor
        //   função 'atan( , )' : arcotangente. Veja https://en.wikipedia.org/wiki/Atan2.
        //   função 'asin( )'   : seno inverso.
        //   constante M_PI
        //   variável position_model

        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;
        vec4 d = position_model - bbox_center;

        float rho   = length(d);
        float theta = atan(d.x,d.z);
        float phi   = asin(d.y / rho);

        U = (theta + M_PI) / 2.0 / M_PI;
        V = (phi + M_PI_2) / M_PI;

		// Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
		Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    }
    else if ( object_id == BUNNY )
    {
        // PREENCHA AQUI as coordenadas de textura do coelho, computadas com
        // projeção planar XY em COORDENADAS DO MODELO. Utilize como referência
        // o slides 99-104 do documento Aula_20_Mapeamento_de_Texturas.pdf,
        // e também use as variáveis min*/max* definidas abaixo para normalizar
        // as coordenadas de textura U e V dentro do intervalo [0,1]. Para
        // tanto, veja por exemplo o mapeamento da variável 'p_v' utilizando
        // 'h' no slides 158-160 do documento Aula_20_Mapeamento_de_Texturas.pdf.
        // Veja também a Questão 4 do Questionário 4 no Moodle.

        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model.x - minx) / (maxx - minx);
        V = (position_model.y - miny) / (maxy - miny);

		// Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
		Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
    }
    else if ( object_id == PLANE )
    {
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        U = texcoords.x;
        V = texcoords.y;

		// Obtemos a refletância difusa a partir da leitura da imagem TextureImage1
		Kd0 = texture(TextureImage1, vec2(U,V)).rgb;
    }
    else if ( object_id == DUNGEON_OBJECT )
    {
        Kd0 = texture(TextureImage2, texcoords).rgb;
    }
    else if ( object_id == ROGUE )
    {
        Kd0 = texture(TextureImage3, texcoords).rgb;
    }
    else if ( object_id == SKELETON )
    {
        Kd0 = texture(TextureImage4, texcoords).rgb;
    }
    else if ( object_id == STAFF )
    {
        Kd0 = texture(TextureImage5, texcoords).rgb;
    }

    vec3 surface_normal = normalize(normal.xyz);
    vec3 view_direction = normalize(camera_position.xyz - p.xyz);
    vec3 lighting = ambient_strength * Kd0;

    for (int i = 0; i < light_count; ++i)
    {
        vec3 to_light = light_positions[i] - p.xyz;
        float distance_to_light = length(to_light);
        vec3 light_direction = normalize(to_light);
        float attenuation = 1.0 / (1.0 + 0.35 * distance_to_light
                                   + 0.22 * distance_to_light * distance_to_light);

        float diffuse_strength = max(dot(surface_normal, light_direction), 0.0);
        vec3 diffuse = Kd0 * light_colors[i] * diffuse_strength;

        vec3 reflected_direction = reflect(-light_direction, surface_normal);
        // Termo especular de Phong: Ks * I * (r . v)^q.
        float phong = pow(max(dot(view_direction, reflected_direction), 0.0), shininess);
        vec3 specular = specular_strength * light_colors[i] * phong;

        lighting += attenuation * (diffuse + specular);
    }

    // Os esqueletos têm um leve brilho próprio (aura de morto-vivo), para
    // continuarem visíveis nas partes escuras da masmorra.
    if ( object_id == SKELETON )
        lighting += 0.18 * Kd0;

    // O cajado é mágico: brilha com luz própria esverdeada.
    if ( object_id == STAFF )
        lighting += Kd0 * vec3(0.5, 1.2, 0.6);

    color.rgb = lighting;

    // Névoa: quanto mais longe da câmera, mais a cor se aproxima da cor da
    // névoa. Fator f = exp(-(densidade * distância)^2), entre 0 e 1. A cor da
    // névoa vem em sRGB (igual à cor de fundo), então a convertemos para o
    // espaço linear antes de misturar (a correção gamma é aplicada no final).
    float fog_distance = length(camera_position.xyz - p.xyz);
    float fog_factor = exp(-pow(fog_density * fog_distance, 2.0));
    color.rgb = mix(pow(fog_color, vec3(2.2)), color.rgb, fog_factor);

    // NOTE: Se você quiser fazer o rendering de objetos transparentes, é
    // necessário:
    // 1) Habilitar a operação de "blending" de OpenGL logo antes de realizar o
    //    desenho dos objetos transparentes, com os comandos abaixo no código C++:
    //      glEnable(GL_BLEND);
    //      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // 2) Realizar o desenho de todos objetos transparentes *após* ter desenhado
    //    todos os objetos opacos; e
    // 3) Realizar o desenho de objetos transparentes ordenados de acordo com
    //    suas distâncias para a câmera (desenhando primeiro objetos
    //    transparentes que estão mais longe da câmera).
    // Alpha default = 1 = 100% opaco = 0% transparente
    color.a = 1;

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color.rgb = pow(color.rgb, vec3(1.0,1.0,1.0)/2.2);

    // Tingimento (captura).
    color.rgb = mix(color.rgb, tint.rgb, tint.a);
} 
