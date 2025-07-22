const fs = require("fs");
const path = require("path");
const xlsx = require("xlsx");

const BASE_DIR = "evolutions";
const algs = ["a1", "a2", "a3"];
const formatos = ["asym", "sym"];

const resultado = [];

function extrairTempoTotal(conteudo) {
  const match = conteudo.match(/tempo total em segundos:\s*([0-9.]+)/);
  return match ? parseFloat(match[1]) : null;
}

algs.forEach(algoritmo => {
  formatos.forEach(formato => {
    const formatoPath = path.join(BASE_DIR, algoritmo, formato);
    if (!fs.existsSync(formatoPath)) return;

    fs.readdirSync(formatoPath).forEach(instancia => {
      const instanciaPath = path.join(formatoPath, instancia);
      if (!fs.statSync(instanciaPath).isDirectory()) return;

      let tempos = [];

      for (let i = 1; i <= 20; i++) {
        const dadosPath = path.join(instanciaPath, `exec_${i}`, "dados");
        if (fs.existsSync(dadosPath)) {
          const conteudo = fs.readFileSync(dadosPath, "utf-8");
          const tempo = extrairTempoTotal(conteudo);
          if (tempo !== null) {
            tempos.push(tempo);
          }
        }
      }

      if (tempos.length > 0) {
        const media = tempos.reduce((a, b) => a + b, 0) / tempos.length;
        resultado.push({
          Algoritmo: algoritmo,
          Formato: formato,
          Instancia: instancia,
          "Tempo Médio (s)": parseFloat(media.toFixed(6)),
        });
      }
    });
  });
});

// Gerar planilha Excel
const wb = xlsx.utils.book_new();
const ws = xlsx.utils.json_to_sheet(resultado);
xlsx.utils.book_append_sheet(wb, ws, "Tempos Médios");
xlsx.writeFile(wb, "tempos_medios.xlsx");

console.log("Arquivo 'tempos_medios.xlsx' gerado com sucesso.");
