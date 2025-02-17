// if you want to use more textures, don't forget to define them there
uniform sampler2D diffuse_map;
//uniform sampler2D diffuse_map2;
//uniform sampler2D diffuse_map3;
//uniform sampler2D diffuse_map4;

varying float fogFactor;

void main()
{
  vec4 color = texture2D( diffuse_map, gl_TexCoord[ 0 ].st ) * gl_Color;
	gl_FragColor = mix(  vec4(gl_Fog.color), color, fogFactor);
}
