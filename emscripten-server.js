import express from "express";
import path from "path";
import { fileURLToPath } from "url";
const app = express();
const PORT = 8080;

const buildDir = path.join(path.dirname(fileURLToPath(import.meta.url)), "build-emscripten");

app.use((req, res, next) => {
  res.setHeader("Cross-Origin-Opener-Policy", "same-origin");
  res.setHeader("Cross-Origin-Embedder-Policy", "require-corp");
  next();
});

app.use(express.static(buildDir));

app.get("/", (req, res) => {
  res.sendFile(path.join(buildDir, "ResourceDragon.html"));
});

app.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}`);
});
