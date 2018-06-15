#pragma once



struct Vertex
{
	glm::vec4 m_v4Position;
	glm::vec2 m_v2UV;
	glm::vec4 m_v4Colour;
};

struct Quad
{
	static const unsigned int	c_uiNoOfIndicies = 6;
	static const unsigned int	c_uiNoOfVerticies = 4;
	Vertex						m_Verticies[c_uiNoOfVerticies];
	unsigned int				m_uiIndicies[c_uiNoOfIndicies];
};

class RenderSample
{
private:
	RenderSample();

	bool Init();
public:
	~RenderSample();

	static RenderSample* Create();

	void Render(const glm::mat4& projection, const glm::mat4& view) const;

	unsigned int	m_shaderId;
	unsigned int	m_textureId;
	unsigned int	g_VBO = 0;
	unsigned int	g_IBO = 0;
	unsigned int	g_VAO = 0;

	glm::mat4	g_ModelMatrix;
};

