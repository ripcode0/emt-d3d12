struct vs_input {
  float3 pos : POSITION;
  float3 col : COLOR;
};

struct vs_output {
  float4 pos : SV_POSITION;
  float3 col : COLOR;
};

vs_output main(vs_input vs) {
  vs_output vo;
  vo.pos = float4(vs.pos, 1);
  vo.col = vs.col;
  return vo;
}