
function createGUI(filename) {
    //var h = new JSROOT.HierarchyPainter('example','FileContentDiv');
    //var h = new JSROOT.HierarchyPainter("example");

    //configure flex layout in Main Div!
    //h.SetDisplay("flex","MainDiv");

    //h.OpenRootFile(filename,function() {
        //h.display();
    //});
    //h.OpenRootFile(filename);
    console.log(filename)
    JSROOT.NewHttpRequest("h_occupancy_cbc0.json", 'object', function(obj) {
         JSROOT.draw("drawing", obj, "hist");
      }).send();
}
