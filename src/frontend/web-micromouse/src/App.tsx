import { Route, Routes } from 'react-router-dom'
import { Sidebar } from './components/Sidebar'
import { LabirintoPage } from './pages/LabirintoPage'
import { TelemetryPage } from './pages/TelemetryPage'
import { HistoryPage } from './pages/HistoryPage'
import { RelatorioPage } from './pages/RelatorioPage'
import { Toaster } from 'sonner';
import { TelemetryProvider } from './contexts/TelemetryProvider';

function App() {
  return (
    <TelemetryProvider>
    <div className="flex h-screen w-screen bg-[#08111F] sm:flex-row flex-col ">
      <Toaster theme="light" position="top-center" richColors />

      <Sidebar/>
      <Routes>
        <Route path="/" element={<TelemetryPage />} />
        <Route path="/labirinto" element={<LabirintoPage />} />
        <Route path="/historico" element={<HistoryPage />} />
        <Route path="/relatorio" element={<RelatorioPage />} />
      </Routes>
    </div>
    </TelemetryProvider>
  );
}

export default App;
