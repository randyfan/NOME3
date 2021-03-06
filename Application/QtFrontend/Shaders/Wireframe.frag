#version 330 core

uniform struct LightInfo { // these variables are defined in WireframeLit.xml
    vec4 position;
    vec3 intensity;
} light;

uniform struct LineInfo {
    float width;
    vec4 color;
} line;

uniform vec3 ka;            // Ambient reflectivity
uniform vec3 kd;            // Diffuse reflectivity
uniform vec3 ks;            // Specular reflectivity
uniform float shininess;    // Specular shininess factor
uniform int showFacets; // binary int. 0 = don't show facets (face edges), 1 = show facets

in WireframeVertex { // these variables are outputted from Wireframe.geom
    vec3 position;
    vec3 normal;
    noperspective vec4 edgeA;
    noperspective vec4 edgeB;
    flat int configuration;
    flat int colorSelected; // Randy added this on 12/3  Also note that integer types are never interpolated. You must declare them as flat in any case. https://stackoverflow.com/questions/27581271/flat-qualifier-in-glsl
    vec3 faceColor;
} fs_in;

out vec4 fragColor;

vec3 adsModel( const in vec3 pos, const in vec3 n )
{
    // Calculate the vector from the light to the fragment
    vec3 s = normalize( vec3( light.position ) - pos );

    // Calculate the vector from the fragment to the eye position (the
    // origin since this is in "eye" or "camera" space
    vec3 v = normalize( -pos );

    // Refleft the light beam using the normal at this fragment
    vec3 r = reflect( -s, n );

    // Calculate the diffus component
    vec3 diffuse = vec3( max( dot( s, n ), 0.0 ) );

    // Calculate the specular component
    vec3 specular = vec3( pow( max( dot( r, v ), 0.0 ), shininess ) );

    // Combine the ambient, diffuse and specular contributions
    return light.intensity * (ka + kd*diffuse); //light.intensity * ( ka + kd * diffuse + ks * specular );
}

vec3 customadsModel( const in vec3 pos, const in vec3 n, const in vec3 customColor)
{
    // Calculate the vector from the light to the fragment
    vec3 s = normalize( vec3( light.position ) - pos );

    // Calculate the vector from the fragment to the eye position (the
    // origin since this is in "eye" or "camera" space
    vec3 v = normalize( -pos );

    // Calculate the diffus component
    vec3 diffuse = vec3( max( dot( s, n ), 0.0 ) );

    // Combine the ambient, diffuse and specular contributions
    return light.intensity * (ka + customColor*diffuse);
}

vec4 shadeLine( const in vec4 color )
{
    // Find the smallest distance between the fragment and a triangle edge
    float d;
    if ( fs_in.configuration == 0 )
    {
        // Common configuration
        d = min( fs_in.edgeA.x, fs_in.edgeA.y );
        d = min( d, fs_in.edgeA.z );
    }
    else
    {
        // Handle configuration where screen space projection breaks down
        // Compute and compare the squared distances
        vec2 AF = gl_FragCoord.xy - fs_in.edgeA.xy;
        float sqAF = dot( AF, AF );
        float AFcosA = dot( AF, fs_in.edgeA.zw );
        d = abs( sqAF - AFcosA * AFcosA );

        vec2 BF = gl_FragCoord.xy - fs_in.edgeB.xy;
        float sqBF = dot( BF, BF );
        float BFcosB = dot( BF, fs_in.edgeB.zw );
        d = min( d, abs( sqBF - BFcosB * BFcosB ) );

        // Only need to care about the 3rd edge for some configurations.
        if ( fs_in.configuration == 1 || fs_in.configuration == 2 || fs_in.configuration == 4 )
        {
            float AFcosA0 = dot( AF, normalize( fs_in.edgeB.xy - fs_in.edgeA.xy ) );
            d = min( d, abs( sqAF - AFcosA0 * AFcosA0 ) );
        }

        d = sqrt( d );
    }

    // Blend between line color and phong color
    float mixVal;
    if ( d < line.width - 1.0 )
    {
        mixVal = 1.0;
    }
    else if ( d > line.width + 1.0 )
    {
        mixVal = 0.0;
    }
    else
    {
        float x = d - ( line.width - 1.0 );
        mixVal = exp2( -2.0 * ( x * x ) );
    }

    return mix( color, line.color, mixVal );
}

void main()
{
    vec4 color;
    const float eps = 0.01; // Needed for floating point error

    // if this face has a special faceColor (999.0 is default, so if facecolor is not 999, we know it's not default)
    if (abs(fs_in.faceColor[1] - 999.0) > eps) {
      color =  vec4( customadsModel( fs_in.position, normalize( fs_in.normal ), fs_in.faceColor ), 1.0 );
    }
    else{ // use instanceColor ("kd")
      // Calculate the color from the phong model. Specular has been removed.
      color =  vec4( adsModel( fs_in.position, normalize( fs_in.normal ) ), 1.0 );
    }


    if (fs_in.colorSelected == 0) { // Randy added this on 12/3
      if (showFacets == 1) {
        fragColor = shadeLine( color );
      }
      else {
        fragColor = color;
      }
    }
    else { // Randy added this on 12/3
      vec4 selectedCol = vec4(0.0, 0.0, 1.0, 1.0);
      fragColor = selectedCol; // Randy added this on 12/3
    } // Randy added this on 12/3

}
