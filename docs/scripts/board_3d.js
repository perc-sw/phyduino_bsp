// "View in 3D": adds a button to every board photo panel with a data-model attribute,
// which swaps the photo for an interactive model of the board.
//
//   <div class="board-img-panel" data-model="./models/phyduino_pico.glb.js">
//     <img src="./images/phyduino_pico.png" alt="...">
//   </div>
//   <link rel="stylesheet" href="./style/board_3d.css">   (in <head>)
//   <script src="./scripts/board_3d.js"></script>          (end of <body>)
//
// The viewer (Google's <model-viewer>) and the model download only when the button is clicked.
//
// Models are KiCad 10 GLB exports wrapped in a script file, so the pages also work when opened
// straight from disk (browsers block loading .glb files there). <name>.glb.js sets
// window.phyduinoModels.<name>. Re-export when the board layout changes, from the board's
// KiCad folder (here ../phyduino_pico):
//   kicad-cli pcb export glb --include-silkscreen --include-soldermask -o phyduino_pico.glb phyduino_pico.kicad_pcb
//   python -c "import base64; d=base64.b64encode(open('phyduino_pico.glb','rb').read()).decode(); open('../phyduino_bsp/docs/models/phyduino_pico.glb.js','w').write('window.phyduinoModels=window.phyduinoModels||{};window.phyduinoModels.phyduino_pico='+chr(34)+'data:model/gltf-binary;base64,'+d+chr(34)+';'+chr(10))"
// then delete phyduino_pico.glb.
(function () {
  var VIEWER = "https://ajax.googleapis.com/ajax/libs/model-viewer/4.3.1/model-viewer.min.js";
  var viewerRequested = false;

  function loadScript(src, isModule, onload, onerror) {
    var script = document.createElement("script");
    if (isModule) script.type = "module";
    script.src = src;
    script.onload = onload || null;
    script.onerror = onerror;
    document.head.appendChild(script);
  }

  // KiCad exports its "Blue" solder mask as a light, 83%-opaque blue, so the pale board
  // material shows through. Repaint it opaque in the photo's blue (about #25469F).
  function darkenSolderMask(model) {
    model.model.materials.forEach(function (material) {
      var c = material.pbrMetallicRoughness.baseColorFactor;
      if (c[3] < 1 && c[2] > c[0] + 0.3) {
        material.pbrMetallicRoughness.setBaseColorFactor([0.019, 0.061, 0.347, 1]);  // linear #25469F
        material.setAlphaMode("OPAQUE");
      }
    });
  }

  function setUp(panel) {
    var modelScript = panel.getAttribute("data-model");
    var name = modelScript.split("/").pop().replace(/\.glb\.js$/, "");
    var photo = panel.querySelector("img");
    var model = null;

    var button = document.createElement("button");
    button.type = "button";
    button.className = "view-3d-btn";
    button.textContent = "View in 3D";
    panel.appendChild(button);

    function show3d(on) {
      photo.hidden = on;
      model.hidden = !on;
      button.textContent = on ? "Show photo" : "View in 3D";
    }

    function fail() {
      if (model) model.remove();
      model = null;
      photo.hidden = false;
      button.textContent = "3D view unavailable";
      button.disabled = true;
    }

    function createModel() {
      var data = window.phyduinoModels && window.phyduinoModels[name];
      if (!data) return fail();
      model = document.createElement("model-viewer");
      model.setAttribute("alt", photo.getAttribute("alt").replace(/, top view$/, "") + ", interactive 3D model");
      model.setAttribute("camera-controls", "");       // drag to rotate, scroll to zoom, right-drag to move
      model.setAttribute("touch-action", "pan-y");     // vertical swipes still scroll the page
      model.setAttribute("camera-orbit", "210deg 50deg 80%");
      model.setAttribute("shadow-intensity", "1");
      model.setAttribute("tone-mapping", "commerce");  // keeps the solder mask colour closer to the photo
      model.setAttribute("exposure", "0.73");          // default is 1; lower is darker
      model.addEventListener("load", function () {
        darkenSolderMask(model);
        button.disabled = false;
        show3d(true);
        button.textContent = "Show photo";
      });
      model.addEventListener("error", fail);
      model.setAttribute("src", data);
      panel.insertBefore(model, button);
      show3d(true);
      button.textContent = "Loading 3D…";
    }

    button.addEventListener("click", function () {
      if (model) return show3d(photo.hidden === false);
      button.disabled = true;
      button.textContent = "Loading 3D…";
      if (!viewerRequested) {
        viewerRequested = true;
        loadScript(VIEWER, true, null, fail);
      }
      loadScript(modelScript, false, createModel, fail);
    });
  }

  Array.prototype.forEach.call(document.querySelectorAll(".board-img-panel[data-model]"), setUp);
})();
