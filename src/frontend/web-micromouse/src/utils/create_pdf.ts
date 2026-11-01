type Execution = {
    id: string;
    maze: string;
    algorithm: string;
    date: string;
    status: string;
    duration: string;
    averageSpeed: string;
    cellsVisited: number;
    averageRpm: number;
    batteryConsumed: string;
    distance: string;
};



function createReportPdf(execution: Execution) {
    const lines = [
        `Relatorio da execucao #${execution.id}`,
        `${execution.maze} - ${execution.algorithm}`,
        `Status: ${execution.status}`,
        `Tempo total: ${execution.duration}`,
        `Velocidade media: ${execution.averageSpeed}`,
        `Celulas visitadas: ${execution.cellsVisited}`,
        `RPM medio: ${execution.averageRpm}`,
        `Bateria consumida: ${execution.batteryConsumed}`,
        `Distancia percorrida: ${execution.distance}`,
        "Conteudo: Resumo executivo, mapa do labirinto explorado, graficos de telemetria,",
        "log de eventos, analise de algoritmo e comparativo com execucoes anteriores.",
    ];
    const safeLines = lines.map((line) => line.normalize("NFD").replace(/[\u0300-\u036f]/g, "").replace(/[^\x20-\x7E]/g, ""));
    const stream = ["BT", "/F1 12 Tf", "50 790 Td", ...safeLines.flatMap((line, index) => [index ? "0 -24 Td" : "", `(${line.replace(/[\\()]/g, "\\$&")}) Tj`]).filter(Boolean), "ET"].join("\n");
    const objects = [
        "<< /Type /Catalog /Pages 2 0 R >>",
        "<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>",
        "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
        `<< /Length ${stream.length} >>\nstream\n${stream}\nendstream`,
    ];
    let pdf = "%PDF-1.4\n";
    const offsets = [0];
    objects.forEach((object, index) => {
        offsets.push(pdf.length);
        pdf += `${index + 1} 0 obj\n${object}\nendobj\n`;
    });
    const xrefOffset = pdf.length;
    pdf += `xref\n0 ${objects.length + 1}\n0000000000 65535 f \n`;
    offsets.slice(1).forEach((offset) => { pdf += `${String(offset).padStart(10, "0")} 00000 n \n`; });
    pdf += `trailer\n<< /Size ${objects.length + 1} /Root 1 0 R >>\nstartxref\n${xrefOffset}\n%%EOF`;
    return new Blob([pdf], { type: "application/pdf" });
}





export {
    createReportPdf
}



 
