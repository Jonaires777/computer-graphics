#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D u_Texture;

void main() {
    vec4 texColor = texture(u_Texture, TexCoord);
    
    // Distância do pixel atual ao centro da tela (0.5, 0.5)
    float dist = distance(TexCoord, vec2(0.5, 0.5));
    
    // Desenha uma mira: um pequeno ponto vermelho no centro
    // Se a distância for menor que 0.005 (ajuste conforme necessário), pinta de vermelho
    if (dist < 0.005) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    } else {
        FragColor = texColor;
    }
}