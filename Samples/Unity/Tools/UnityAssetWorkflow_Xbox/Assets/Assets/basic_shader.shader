Shader "BasicTestShader"
{
    Properties
    {
        _MainTex ("Texture", 2D) = "white" {}
        _MainColor ("Color", Color) = (1,1,1,1)
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }

        Pass
        {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag

            #include "UnityCG.cginc"

            struct appdata
            {
                float4 vertex : POSITION;
                float2 uv : TEXCOORD0;
            };

            struct vertdata
            {
                float2 uv : TEXCOORD0;
                float4 vertex : SV_POSITION;
            };

            sampler2D _MainTex;
            float4 _MainTex_ST;
            float4 _MainColor;

            vertdata vert (appdata v)
            {
                vertdata o;
                o.vertex = UnityObjectToClipPos(v.vertex);
                o.uv = TRANSFORM_TEX(v.uv, _MainTex);
                return o;
            }

            fixed4 frag (vertdata i) : SV_Target
            {
                // sample the texture
                fixed4 col = tex2D(_MainTex, i.uv);

                // do an addition with the MainColor property
                col = col + (_MainColor * 0.5);

                // NOTE: uncomment to instead do a multiply with the MainColor property
                //col = col * _MainColor;

                return col;
            }
            ENDCG
        }
    }
}
