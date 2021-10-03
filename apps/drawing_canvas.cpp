#include "drawing_canvas.hh"

void showDrawingCanvas(){
	// keep track of canvas data
	static std::vector<ImVec2> canvasPoints;
	
	// use this to keep track of where each separately drawn segement ends because everything gets redrawn each re-render from the beginning
	static std::set<int> lastSegmentIndexes;
	
	// keep track of colors
	static std::vector<ImU32> canvasPointColors;
    
	// brush size
	static float brushSize = 3.0f;
	
	// color wheel stuff
	static ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); // note this isn't assignment but just initialization
	static bool refColor = false;
	static ImVec4 refColor_v(1.0f, 0.0f, 1.0f, 0.5f);
	ImGuiColorEditFlags flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR;
	ImU32 selectedColor = ImColor(color);
	
	//static ImVec2 scrolling(0.0f, 0.0f);
	
	ImGui::BeginChild("drawing canvas", ImVec2(0, 0), true);
	ImGui::Columns(2, "stuff");
	
	ImVec2 canvasPos0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
	ImVec2 canvasSize = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
	ImVec2 canvasPos1 = ImVec2(canvasSize.x*0.95f, canvasSize.y*0.95f);

	// Draw border and background color
	ImGuiIO& io = ImGui::GetIO();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(canvasPos0, canvasPos1, IM_COL32(50, 50, 50, 255));
	drawList->AddRect(canvasPos0, canvasPos1, IM_COL32(255, 255, 255, 255));

	// This will catch our interactions
	ImGui::InvisibleButton("drawingCanvas", canvasPos1, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
	
	// Hovered - this is so we ensure that we take into account only mouse interactions that occur
	// on this particular canvas. otherwise it could pick up mouse clicks that occur on other windows as well.
	const bool isHovered = ImGui::IsItemHovered();
	const bool isActive = ImGui::IsItemActive();   // Held
	
	const ImVec2 origin(canvasPos0.x, canvasPos0.y); // Lock scrolled origin
	const ImVec2 mousePosInCanvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

	if (isActive && isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
	{
		canvasPoints.push_back(mousePosInCanvas);
		canvasPointColors.push_back(selectedColor);
	}
	
	ImVec2 dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
	if (isHovered && isActive && (dragDelta.x > 0 || dragDelta.y > 0))
	{
		canvasPoints.push_back(mousePosInCanvas);
		canvasPointColors.push_back(selectedColor);
	}
	
	int lastIdx = -1;
	if (isHovered && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		// I think we might be setting the last element in the canvasPoints vector to
		// mousePosInCanvas? and since it's a reference, it works? don't think we need this though
		//canvasPoints.back() = mousePosInCanvas;
		lastIdx = canvasPoints.size() - 1;
	}
	
	// need to redraw each event update
	int numPoints = (int)canvasPoints.size();
	for(int i = 0; i < numPoints; i++){
		ImVec2 p1 = ImVec2(origin.x + canvasPoints[i].x, origin.y + canvasPoints[i].y);
		ImVec2 p2 = ImVec2(origin.x + canvasPoints[i+1].x, origin.y + canvasPoints[i+1].y);
		ImU32 theColor = canvasPointColors[i];
		
		if(lastSegmentIndexes.find(i) != lastSegmentIndexes.end()){
			//std::cout << "last point index: " << lastPointIndex << std::endl;				
			// don't draw a line from this point to the next
			drawList->AddCircleFilled(p1, brushSize-1.5f, theColor, 8.0f);
			continue;
		}
		if(i < numPoints - 1){
			// connect the points
			drawList->AddLine(p1, p2, theColor, brushSize);
		}else{
			// just draw point
			drawList->AddCircle(p1, brushSize-1.5f, theColor, 8.0f);
		}
	}
	
	if(lastIdx > -1){
		// keep track of the last point of the last segment drawn so we don't connect two separately drawn segments
		lastSegmentIndexes.insert(lastIdx); //lastPointIndex = lastIdx;
	}
	
	ImGui::NextColumn();
	
	// show color wheel
	ImGui::ColorPicker4("MyColor", (float*)&color, flags, refColor ? &refColor_v.x : NULL);
	
	ImGui::Dummy(ImVec2(0.0f, 5.0f)); // add some vertical spacing
	
	ImGui::SliderFloat("brush size", &brushSize, 1.0f, 10.0f);
    
	ImGui::Dummy(ImVec2(0.0f, 5.0f));
    
	if(ImGui::Button("clear")){
		canvasPoints.clear();
		lastSegmentIndexes.clear();
		canvasPointColors.clear();
	}
	
	ImGui::Columns();
	
	ImGui::EndChild();
}