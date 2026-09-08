// Fixture occluder: only sample0 receives depth; other MSAA samples stay open.
void main(out uint coverage : SV_Coverage) { coverage = 1; }
