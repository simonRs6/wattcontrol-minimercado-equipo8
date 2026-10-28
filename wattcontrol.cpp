#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

// Directiva exigida por la actividad
#define HORAS_DIA 24

// Constantes del Minimercado (Negocio 8)
const float POTENCIA_CONTRATADA = 12.0; // kW
const float PRECIO_CARO = 1200.0;       // COP
const int ALERTA_EMBALSES = 40;         // Porcentaje
const int HORA_PICO_INICIO = 18;        // 6 PM
const int HORA_PICO_FIN = 20;           // 8 PM (cubre hasta 20:59)

// ==========================================
// 1. CLASE BASE ABSTRACTA Y HERENCIA
// ==========================================
class Carga {
protected:
    string nombre;
    float potenciaKW;
    bool encendida;
    int horasUso;

public:
    Carga(string n, float p) : nombre(n), potenciaKW(p), encendida(false), horasUso(0) {}
    virtual ~Carga() {}
    
    // Método virtual puro que garantiza el Polimorfismo
    virtual bool debeEncender(float precio, int embalses, int hora) = 0;

    float getPotenciaKW() { return potenciaKW; }
    bool isEncendida() { return encendida; }
    int getHorasUso() { return horasUso; }
    string getNombre() { return nombre; }
    void setEncendida(bool estado) { encendida = estado; }
    void incrementarHorasUso() { horasUso++; }
};

class CargaCritica : public Carga {
public:
    CargaCritica(string n, float p) : Carga(n, p) {}
    
    bool debeEncender(float precio, int embalses, int hora) override {
        // Regla 1: La carga crítica nunca se apaga
        return true; 
    }
};

class CargaAplazable : public Carga {
private:
    int horasMinimas;

public:
    CargaAplazable(string n, float p, int hMin) : Carga(n, p), horasMinimas(hMin) {}
    
    bool debeEncender(float precio, int embalses, int hora) override {
        // Reglas 2 a 5: Condición compuesta con operadores lógicos
        bool esHoraPico = (hora >= HORA_PICO_INICIO && hora <= HORA_PICO_FIN);
        
        if (embalses < ALERTA_EMBALSES || esHoraPico || precio > PRECIO_CARO) {
            return false; // Se apaga o aplaza
        }
        return true; // Condiciones normales, se enciende
    }
    
    int getHorasMinimas() { return horasMinimas; }
};

// ==========================================
// 2. CLASES DE SOPORTE PARA RELACIONES
// ==========================================
class Medidor {
private:
    float acumuladoKWh;
    float costoTotal;

public:
    Medidor() : acumuladoKWh(0.0), costoTotal(0.0) {}
    
    void registrarConsumo(float kWh, float costo) {
        acumuladoKWh += kWh;
        costoTotal += costo;
    }
    
    float getAcumuladoKWh() { return acumuladoKWh; }
    float getCostoTotal() { return costoTotal; }
};

class Mercado {
private:
    float precioActual;
    int nivelEmbalses;

public:
    Mercado() : precioActual(0.0), nivelEmbalses(0) {}
    
    void setCondiciones(float precio, int embalses) {
        precioActual = precio;
        nivelEmbalses = embalses;
    }
    
    void generarCondicionesAleatorias(int hora) {
        // Simulador de precios realistas para el mercado energético (entre 400 y 1600 COP)
        precioActual = 400.0f + static_cast<float>(rand() % 1201);
        // Simulador de embalses (entre 20% y 100%)
        nivelEmbalses = 20 + (rand() % 81);
    }
    
    float getPrecioActual() { return precioActual; }
    int getNivelEmbalses() { return nivelEmbalses; }
};
// ==========================================
// 3. CLASE NEGOCIO (AGREGACIÓN, COMPOSICIÓN Y ASOCIACIÓN)
// ==========================================
class Negocio {
private:
    string nombre;
    float potenciaContratada;
    Carga* cargas[3];      // Agregación: Arreglo de punteros a Carga
    Medidor medidor;       // Composición: El medidor es parte del negocio
    Mercado* mercado;      // Asociación: Puntero al mercado
    
    // Variables para el resumen
    float costoSinGestor;
    bool alertaProduccionIncompleta;

public:
    Negocio(string n, float p, Mercado* m) : nombre(n), potenciaContratada(p), mercado(m), costoSinGestor(0.0), alertaProduccionIncompleta(false) {
        for (int i = 0; i < 3; i++) {
            cargas[i] = nullptr;
        }
    }
    
    void agregarCarga(Carga* c, int indice) {
        if (indice >= 0 && indice < 3) {
            cargas[indice] = c;
        }
    }
    
    void evaluarCargas(int hora) {
        float potenciaActual = 0.0;
        float precio = mercado->getPrecioActual();
        int embalses = mercado->getNivelEmbalses();
        
        // Fase 1: Cada carga decide de forma independiente (Polimorfismo)
        for (int i = 0; i < 3; i++) {
            if (cargas[i] != nullptr) {
                bool decision = cargas[i]->debeEncender(precio, embalses, hora);
                cargas[i]->setEncendida(decision);
                if (decision) {
                    potenciaActual += cargas[i]->getPotenciaKW();
                }
            }
        }
        
        // Fase 2: Regla 6 del Negocio (Control de Potencia Contratada)
        if (potenciaActual > potenciaContratada) {
            cout << "  [ALERTA] Potencia actual (" << potenciaActual << " kW) supera la contratada (" << potenciaContratada << " kW).\n";
            
            Carga* cargaMayor = nullptr;
            float mayorPotencia = 0.0;
            
            for (int i = 0; i < 3; i++) {
                if (cargas[i] != nullptr && cargas[i]->isEncendida()) {
                    // Identificar si es una carga aplazable mediante dynamic_cast
                    CargaAplazable* aplazable = dynamic_cast<CargaAplazable*>(cargas[i]);
                    if (aplazable != nullptr && cargas[i]->getPotenciaKW() > mayorPotencia) {
                        mayorPotencia = cargas[i]->getPotenciaKW();
                        cargaMayor = cargas[i];
                    }
                }
            }
            
            if (cargaMayor != nullptr) {
                cargaMayor->setEncendida(false);
                potenciaActual -= cargaMayor->getPotenciaKW();
                cout << "  [ACCION] Se apago '" << cargaMayor->getNombre() << "' para evitar penalidad.\n";
            }
        }
        
        // Fase 3: Registrar uso y consumo
        float costoHoraSinGestor = 0.0;
        for (int i = 0; i < 3; i++) {
            if (cargas[i] != nullptr) {
                // Costo real con gestor inteligente
                if (cargas[i]->isEncendida()) {
                    cargas[i]->incrementarHorasUso();
                    float costoCarga = cargas[i]->getPotenciaKW() * precio;
                    medidor.registrarConsumo(cargas[i]->getPotenciaKW(), costoCarga);
                }
                // Costo simulado sin gestor (todas encendidas)
                costoHoraSinGestor += cargas[i]->getPotenciaKW() * precio;
            }
        }
        costoSinGestor += costoHoraSinGestor;
    }
    
    void verificarProduccion() {
        alertaProduccionIncompleta = false;
        for (int i = 0; i < 3; i++) {
            if (cargas[i] != nullptr) {
                CargaAplazable* aplazable = dynamic_cast<CargaAplazable*>(cargas[i]);
                if (aplazable != nullptr) {
                    if (aplazable->getHorasUso() < aplazable->getHorasMinimas()) {
                        alertaProduccionIncompleta = true;
                    }
                }
            }
        }
    }
    
    void mostrarResumen() {
        cout << "\n=========================================\n";
        cout << "   RESUMEN DE OPERACION: " << nombre << "\n";
        cout << "=========================================\n";
        
        for (int i = 0; i < 3; i++) {
            if (cargas[i] != nullptr) {
                float kWhConsumidos = cargas[i]->getPotenciaKW() * cargas[i]->getHorasUso();
                cout << "- " << cargas[i]->getNombre() << ": " 
                     << cargas[i]->getHorasUso() << " horas uso | " 
                     << kWhConsumidos << " kWh acumulados.\n";
            }
        }
        
        cout << "-----------------------------------------\n";
        cout << "Costo con WattControl: $" << medidor.getCostoTotal() << "\n";
        cout << "Costo sin gestor:      $" << costoSinGestor << "\n";
        
        float ahorro = costoSinGestor - medidor.getCostoTotal();
        float porcentaje = (costoSinGestor > 0) ? (ahorro / costoSinGestor) * 100.0 : 0.0;
        
        cout << "Ahorro total:          $" << ahorro << " (" << porcentaje << "%)\n";
        
        if (alertaProduccionIncompleta) {
            cout << "\n[ALERTA CRITICA] Produccion incompleta. Las cargas aplazables no cumplieron sus horas minimas.\n";
        }
        cout << "=========================================\n";
    }
    
    void reiniciarSimulacion() {
        costoSinGestor = 0.0;
        alertaProduccionIncompleta = false;
        medidor = Medidor(); // Reinicia el medidor
        // Reiniciar horas de uso no es posible sin modificar Carga, así que asumimos una ejecución por programa
    }
};
// ==========================================
// 4. FUNCION PRINCIPAL Y MENU
// ==========================================
int main() {
    srand(static_cast<unsigned int>(time(0)));
    
    Mercado mercado;
    Negocio minimercado("Minimercado", POTENCIA_CONTRATADA, &mercado);
    
    CargaCritica neveras("Neveras Lacteos y Carnes", 5.0);
    CargaAplazable aire("Aire Acondicionado Central", 6.0, 6);
    CargaAplazable letrero("Letrero y Luces Exteriores", 2.0, 4);
    
    minimercado.agregarCarga(&neveras, 0);
    minimercado.agregarCarga(&aire, 1);
    minimercado.agregarCarga(&letrero, 2);
    
    int opcion;
    int horaManual = 0, embalsesManual = 100;
    float precioManual = 500.0;
    
    do {
        cout << "\n========== MENU WATTCONTROL ==========\n";
        cout << "1. Ingresar lectura manual (hora, precio, embalses)\n";
        cout << "2. Evaluar decision de cada carga (Lectura actual)\n";
        cout << "3. Simular un dia completo (24 h)\n";
        cout << "4. Ver resumen de la ultima simulacion\n";
        cout << "0. Salir\n";
        cout << "Ingrese una opcion: ";
        cin >> opcion;
        
        switch(opcion) {
            case 1:
                do {
                    cout << "Ingrese la hora (0-23): ";
                    cin >> horaManual;
                } while (horaManual < 0 || horaManual > 23);
                
                do {
                    cout << "Ingrese el precio COP (mayor a 0): ";
                    cin >> precioManual;
                } while (precioManual <= 0);
                
                do {
                    cout << "Ingrese % de embalses (0-100): ";
                    cin >> embalsesManual;
                } while (embalsesManual < 0 || embalsesManual > 100);
                
                mercado.setCondiciones(precioManual, embalsesManual);
                cout << "Lectura actualizada correctamente.\n";
                break;
                
            case 2:
                cout << "\nEvaluando hora " << horaManual << " (Precio: $" << mercado.getPrecioActual() << ", Embalses: " << mercado.getNivelEmbalses() << "%)...\n";
                minimercado.evaluarCargas(horaManual);
                break;
                
            case 3:
                cout << "\nIniciando simulacion automatica de " << HORAS_DIA << " horas...\n";
                minimercado.reiniciarSimulacion();
                
                for (int h = 0; h < HORAS_DIA; h++) {
                    mercado.generarCondicionesAleatorias(h);
                    
                    // Condicion Continue: 1 de cada 20 horas falla la lectura (5% probabilidad)
                    if (rand() % 20 == 0) {
                        cout << "Hora " << h << ": [ERROR] Falla lectura del mercado. Solo opera carga critica. Pasando a siguiente hora...\n";
                        neveras.setEncendida(true);
                        neveras.incrementarHorasUso();
                        continue;
                    }
                    
                    // Condicion Break: Emergencia de racionamiento
                    if (mercado.getPrecioActual() > 1500 && mercado.getNivelEmbalses() < 40) {
                        cout << "Hora " << h << ": [EMERGENCIA] Precio > 1500 y embalses < 40%. Racionamiento estricto. Abortando simulacion.\n";
                        break; 
                    }
                    
                    cout << "Hora " << h << " | Precio: $" << mercado.getPrecioActual() << " | Embalses: " << mercado.getNivelEmbalses() << "% -> Evaluando...\n";
                    minimercado.evaluarCargas(h);
                }
                
                minimercado.verificarProduccion();
                cout << "Simulacion finalizada.\n";
                break;
                
            case 4:
                minimercado.mostrarResumen();
                break;
                
            case 0:
                cout << "Saliendo del sistema. Hasta pronto.\n";
                break;
                
            default:
                cout << "Opcion no valida.\n";
        }
    } while (opcion != 0);

    return 0;
}