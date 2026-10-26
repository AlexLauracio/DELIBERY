#include <iostream>
#include <string>
#include <fstream>
#include <cmath>
using namespace std;

const int MAX_ESTABLECIMIENTOS = 30, MAX_CLIENTES = 50, MAX_MENSAJEROS = 30, MAX_ORDENES = 100;
const int MAX_ITEMS_MENU = 15, MAX_LINEAS_ORDEN = 10, MAX_HISTORIAL = 10;
const string NOMBRES_ESTADO[4] = {"Pendiente", "Preparando", "En camino", "Entregado"};
const double TARIFA_BASE = 3.0, COSTO_POR_KM = 1.5, TARIFA_MINIMA = 5.0;

int textoAEntero(const string& texto) {
    int valor = 0, signo = 1;
    for (size_t i = 0; i < texto.size(); i++) {
        if (texto[i] == '-' && valor == 0) signo = -1;
        else if (texto[i] >= '0' && texto[i] <= '9') valor = valor * 10 + (texto[i] - '0');
    }
    return valor * signo;
}
double textoADecimal(const string& texto) {
    double parteEntera = 0, parteDecimal = 0, pesoDecimal = 0.1;
    int signo = 1; bool pasoElPunto = false;
    for (size_t i = 0; i < texto.size(); i++) {
        char c = texto[i];
        if (c == '-') signo = -1;
        else if (c == '.' || c == ',') pasoElPunto = true;
        else if (c >= '0' && c <= '9') {
            if (!pasoElPunto) parteEntera = parteEntera * 10 + (c - '0');
            else { parteDecimal += (c - '0') * pesoDecimal; pesoDecimal /= 10; }
        }
    }
    return signo * (parteEntera + parteDecimal);
}
bool esNumeroValido(const string& texto) {
    bool hayDigito = false;
    for (size_t i = 0; i < texto.size(); i++) {
        char c = texto[i];
        if (c >= '0' && c <= '9') hayDigito = true;
        else if (c != '-' && c != '.' && c != ',') return false;
    }
    return hayDigito;
}
string formatoDosDecimales(double numero) {
    long centavos = (long)(numero * 100 + 0.5);
    return to_string(centavos / 100) + "." + (centavos % 100 < 10 ? "0" : "") + to_string(centavos % 100);
}
int separarPorBarra(const string& linea, string partes[], int maxPartes) {
    int cantidad = 0; string actual;
    for (size_t i = 0; i <= linea.size(); i++) {
        if (i == linea.size() || linea[i] == '|') {
            if (cantidad < maxPartes) partes[cantidad++] = actual;
            actual = "";
        } else actual += linea[i];
    }
    return cantidad;
}
string quitarBarras(string texto) {
    for (size_t i = 0; i < texto.size(); i++) if (texto[i] == '|') texto[i] = ' ';
    return texto;
}
string rellenar(const string& texto, int ancho) {
    string resultado = texto;
    while ((int)resultado.size() < ancho) resultado += ' ';
    return resultado;
}
string centrar(const string& texto, int ancho) {
    int falta = ancho - (int)texto.size();
    if (falta < 0) falta = 0;
    int izquierda = falta / 2;
    return string(izquierda, ' ') + texto + string(falta - izquierda, ' ');
}

struct Ubicacion {
    double latitud, longitud;
    Ubicacion(double lat = 0, double lon = 0) : latitud(lat), longitud(lon) {}
    double distanciaKm(const Ubicacion& otra) const {
        double difLatitud = latitud - otra.latitud, difLongitud = longitud - otra.longitud;
        return sqrt(difLatitud * difLatitud + difLongitud * difLongitud) * 111.0;
    }
    string comoTexto() const { return "(" + to_string(latitud) + ", " + to_string(longitud) + ")"; }
};

class Entidad {
    int id; string nombre; Ubicacion ubicacion;
public:
    Entidad() : id(0) {}
    virtual ~Entidad() {}
    void definir(int nuevoId, const string& nuevoNombre, const Ubicacion& nuevaUbicacion) {
        id = nuevoId; nombre = nuevoNombre; ubicacion = nuevaUbicacion;
    }
    int getId() const { return id; }
    string getNombre() const { return nombre; }
    Ubicacion getUbicacion() const { return ubicacion; }
    void setUbicacion(const Ubicacion& nueva) { ubicacion = nueva; }

    virtual string tipo() const = 0;
    virtual string descripcion() const { return tipo() + ": " + nombre + " " + ubicacion.comoTexto(); }
    virtual void escribir(ostream& salida) const {
        salida << id << "|" << nombre << "|" << ubicacion.latitud << "|" << ubicacion.longitud;
    }
};

void escribirLinea(ostream& salida, const Entidad& entidad) {
    entidad.escribir(salida);
    salida << "\n";
}

class Cliente : public Entidad {
public:
    string tipo() const override { return "Cliente"; }
};

struct ItemMenu { string nombre; double precio; ItemMenu() : precio(0) {} };

class Establecimiento : public Entidad {
    ItemMenu items[MAX_ITEMS_MENU]; int cantidadItems;
public:
    Establecimiento() : cantidadItems(0) {}
    string tipo() const override { return "Establecimiento"; }
    bool agregarItem(const string& nombreItem, double precio) {
        if (cantidadItems >= MAX_ITEMS_MENU) return false;
        items[cantidadItems].nombre = nombreItem; items[cantidadItems].precio = precio;
        cantidadItems++; return true;
    }
    int getCantidadItems() const { return cantidadItems; }
    const ItemMenu& getItem(int posicion) const { return items[posicion]; }
};

class Mensajero : public Entidad {
    string vehiculo; bool ocupado;
public:
    Mensajero() : ocupado(false) {}
    string tipo() const override { return "Mensajero"; }
    string descripcion() const override {
        return tipo() + ": " + getNombre() + " [" + vehiculo + "] " + getUbicacion().comoTexto();
    }
    void escribir(ostream& salida) const override {
        Entidad::escribir(salida);
        salida << "|" << vehiculo << "|" << ocupado;
    }
    void setVehiculo(const string& nuevoVehiculo) { vehiculo = nuevoVehiculo; }
    string getVehiculo() const { return vehiculo; }
    bool estaOcupado() const { return ocupado; }
    void setOcupado(bool valor) { ocupado = valor; }
};

class Orden {
    int idOrden, idCliente, idEstablecimiento, idMensajero, estado, cantidadLineas, cantidadHistorial;
    int posicionItem[MAX_LINEAS_ORDEN], cantidadPedida[MAX_LINEAS_ORDEN];
    double subtotal, tarifaEnvio, distanciaKm;
    string historial[MAX_HISTORIAL];
public:
    Orden() : idOrden(0), idCliente(0), idEstablecimiento(0), idMensajero(0), estado(0),
              cantidadLineas(0), cantidadHistorial(0), subtotal(0), tarifaEnvio(0), distanciaKm(0) {}

    void iniciar(int nuevoId, int cliente, int establecimiento) {
        *this = Orden();
        idOrden = nuevoId; idCliente = cliente; idEstablecimiento = establecimiento;
    }
    bool agregarLinea(int posicion, int cantidad, double precioUnitario) {
        if (cantidadLineas >= MAX_LINEAS_ORDEN) return false;
        posicionItem[cantidadLineas] = posicion;
        cantidadPedida[cantidadLineas] = cantidad;
        subtotal += precioUnitario * cantidad;
        cantidadLineas++;
        return true;
    }
    void definirEnvio(double distancia, double tarifa) { distanciaKm = distancia; tarifaEnvio = tarifa; }
    void asignarMensajero(int idNuevoMensajero) { idMensajero = idNuevoMensajero; }
    void avanzarEstado() { if (estado < 3) estado++; }
    void agregarAlHistorial(const string& texto) { if (cantidadHistorial < MAX_HISTORIAL) historial[cantidadHistorial++] = texto; }

    void restaurar(int id, int cliente, int establecimiento, int mensajero, int nuevoEstado, double nuevoSubtotal, double tarifa, double distancia) {
        *this = Orden();
        idOrden = id; idCliente = cliente; idEstablecimiento = establecimiento; idMensajero = mensajero;
        estado = nuevoEstado; subtotal = nuevoSubtotal; tarifaEnvio = tarifa; distanciaKm = distancia;
    }
    void restaurarLinea(int posicion, int cantidad) {
        if (cantidadLineas >= MAX_LINEAS_ORDEN) return;
        posicionItem[cantidadLineas] = posicion; cantidadPedida[cantidadLineas] = cantidad; cantidadLineas++;
    }

    int getIdOrden() const { return idOrden; }
    int getIdCliente() const { return idCliente; }
    int getIdEstablecimiento() const { return idEstablecimiento; }
    int getIdMensajero() const { return idMensajero; }
    bool tieneMensajero() const { return idMensajero != 0; }
    int getEstado() const { return estado; }
    int getCantidadLineas() const { return cantidadLineas; }
    int getPosicionItem(int linea) const { return posicionItem[linea]; }
    int getCantidadPedida(int linea) const { return cantidadPedida[linea]; }
    double getSubtotal() const { return subtotal; }
    double getTarifaEnvio() const { return tarifaEnvio; }
    double getDistanciaKm() const { return distanciaKm; }
    double getTotal() const { return subtotal + tarifaEnvio; }
    int getCantidadHistorial() const { return cantidadHistorial; }
    string getHistorial(int posicion) const { return historial[posicion]; }
};

class Servicio;

class Almacen {
public:
    static void guardar(const Servicio& servicio);
    static void cargar(Servicio& servicio);
    static void registrarTransaccion(const string& accion);
};

class Servicio {
    Cliente clientes[MAX_CLIENTES]; Establecimiento establecimientos[MAX_ESTABLECIMIENTOS];
    Mensajero mensajeros[MAX_MENSAJEROS]; Orden ordenes[MAX_ORDENES];
    int totalClientes, totalEstablecimientos, totalMensajeros, totalOrdenes;
    friend class Almacen;
public:
    Servicio() : totalClientes(0), totalEstablecimientos(0), totalMensajeros(0), totalOrdenes(0) { Almacen::cargar(*this); }

    int cantidadClientes() const { return totalClientes; }
    int cantidadEstablecimientos() const { return totalEstablecimientos; }
    int cantidadMensajeros() const { return totalMensajeros; }
    int cantidadOrdenes() const { return totalOrdenes; }
    bool existeCliente(int id) const { return id >= 1 && id <= totalClientes; }
    bool existeEstablecimiento(int id) const { return id >= 1 && id <= totalEstablecimientos; }
    bool existeOrden(int id) const { return id >= 1 && id <= totalOrdenes; }
    const Cliente& cliente(int id) const { return clientes[id - 1]; }
    const Establecimiento& establecimiento(int id) const { return establecimientos[id - 1]; }
    const Mensajero& mensajero(int id) const { return mensajeros[id - 1]; }
    const Orden& orden(int id) const { return ordenes[id - 1]; }

    int registrarCliente(const string& nombre, const Ubicacion& ubicacion) {
        if (totalClientes >= MAX_CLIENTES) return 0;
        clientes[totalClientes].definir(totalClientes + 1, quitarBarras(nombre), ubicacion); totalClientes++;
        Almacen::registrarTransaccion("REG_CLIENTE|" + to_string(totalClientes)); Almacen::guardar(*this); return totalClientes;
    }
    int registrarEstablecimiento(const string& nombre, const Ubicacion& ubicacion) {
        if (totalEstablecimientos >= MAX_ESTABLECIMIENTOS) return 0;
        establecimientos[totalEstablecimientos].definir(totalEstablecimientos + 1, quitarBarras(nombre), ubicacion); totalEstablecimientos++;
        Almacen::registrarTransaccion("REG_ESTABLECIMIENTO|" + to_string(totalEstablecimientos)); Almacen::guardar(*this); return totalEstablecimientos;
    }
    int registrarMensajero(const string& nombre, const string& vehiculo, const Ubicacion& ubicacion) {
        if (totalMensajeros >= MAX_MENSAJEROS) return 0;
        mensajeros[totalMensajeros].definir(totalMensajeros + 1, quitarBarras(nombre), ubicacion);
        mensajeros[totalMensajeros].setVehiculo(quitarBarras(vehiculo)); totalMensajeros++;
        Almacen::registrarTransaccion("REG_MENSAJERO|" + to_string(totalMensajeros)); Almacen::guardar(*this); return totalMensajeros;
    }
    bool agregarItemAlMenu(int idEstablecimiento, const string& nombreItem, double precio) {
        if (!existeEstablecimiento(idEstablecimiento) || precio <= 0) return false;
        if (!establecimientos[idEstablecimiento - 1].agregarItem(quitarBarras(nombreItem), precio)) return false;
        Almacen::registrarTransaccion("ADD_ITEM|" + to_string(idEstablecimiento) + "|" + quitarBarras(nombreItem));
        Almacen::guardar(*this); return true;
    }

    double calcularTarifa(double km) const {
        double tarifa = TARIFA_BASE + COSTO_POR_KM * km;
        return tarifa < TARIFA_MINIMA ? TARIFA_MINIMA : tarifa;
    }
    int crearOrden(int idCliente, int idEstablecimiento, const int itemsElegidos[], const int cantidades[], int cantidadElegida) {
        if (!existeCliente(idCliente) || !existeEstablecimiento(idEstablecimiento)) return 0;
        if (cantidadElegida < 1 || totalOrdenes >= MAX_ORDENES) return 0;
        Orden& orden = ordenes[totalOrdenes];
        orden.iniciar(totalOrdenes + 1, idCliente, idEstablecimiento);
        const Establecimiento& local = establecimientos[idEstablecimiento - 1];
        for (int i = 0; i < cantidadElegida; i++) {
            if (itemsElegidos[i] < 1 || itemsElegidos[i] > local.getCantidadItems() || cantidades[i] < 1) continue;
            if (!orden.agregarLinea(itemsElegidos[i] - 1, cantidades[i], local.getItem(itemsElegidos[i] - 1).precio)) break;
        }
        if (orden.getCantidadLineas() == 0) return 0;
        double distancia = local.getUbicacion().distanciaKm(clientes[idCliente - 1].getUbicacion());
        orden.definirEnvio(distancia, calcularTarifa(distancia));
        orden.agregarAlHistorial("Pendiente: orden creada"); totalOrdenes++;
        Almacen::registrarTransaccion("CREAR_ORDEN|" + to_string(orden.getIdOrden())); Almacen::guardar(*this); return orden.getIdOrden();
    }

    string asignarMensajero(int idOrden) {
        if (!existeOrden(idOrden)) return "Orden inexistente.";
        Orden& orden = ordenes[idOrden - 1];
        if (orden.tieneMensajero()) return "La orden ya tiene mensajero.";
        if (orden.getEstado() >= 2) return "La orden ya no admite asignacion.";
        int posicionMasCercano = -1; double menorDistancia = 0;
        Ubicacion ubicacionLocal = establecimientos[orden.getIdEstablecimiento() - 1].getUbicacion();
        for (int i = 0; i < totalMensajeros; i++) {
            if (mensajeros[i].estaOcupado()) continue;
            double distancia = mensajeros[i].getUbicacion().distanciaKm(ubicacionLocal);
            if (posicionMasCercano < 0 || distancia < menorDistancia) { posicionMasCercano = i; menorDistancia = distancia; }
        }
        if (posicionMasCercano < 0) return "No hay mensajeros disponibles.";
        mensajeros[posicionMasCercano].setOcupado(true);
        orden.asignarMensajero(posicionMasCercano + 1);
        orden.agregarAlHistorial("Asignado a " + mensajeros[posicionMasCercano].getNombre());
        Almacen::registrarTransaccion("ASIGNAR|" + to_string(idOrden) + "|" + to_string(orden.getIdMensajero())); Almacen::guardar(*this);
        return "Mensajero " + mensajeros[posicionMasCercano].getNombre() + " asignado (a " + formatoDosDecimales(menorDistancia) + " km del establecimiento).";
    }
    string avanzarEstado(int idOrden) {
        if (!existeOrden(idOrden)) return "Orden inexistente.";
        Orden& orden = ordenes[idOrden - 1];
        if (orden.getEstado() == 3) return "La orden ya fue entregada.";
        if (orden.getEstado() == 1 && !orden.tieneMensajero()) return "Error: asigne un mensajero antes de pasar a 'En camino'.";
        orden.avanzarEstado();
        orden.agregarAlHistorial(NOMBRES_ESTADO[orden.getEstado()]);
        if (orden.getEstado() == 3) {
            Mensajero& mensajeroAsignado = mensajeros[orden.getIdMensajero() - 1];
            mensajeroAsignado.setOcupado(false);
            mensajeroAsignado.setUbicacion(clientes[orden.getIdCliente() - 1].getUbicacion());
        }
        Almacen::registrarTransaccion("ESTADO|" + to_string(idOrden) + "|" + NOMBRES_ESTADO[orden.getEstado()]); Almacen::guardar(*this);
        return "Estado actual: " + NOMBRES_ESTADO[orden.getEstado()];
    }
};

void Almacen::registrarTransaccion(const string& accion) {
    ofstream archivo("transacciones.txt", ios::app);
    archivo << accion << "\n";
}

void Almacen::guardar(const Servicio& s) {
    ofstream archivo; archivo.precision(9);
    archivo.open("clientes.txt");
    for (int i = 0; i < s.totalClientes; i++) escribirLinea(archivo, s.clientes[i]);
    archivo.close(); archivo.open("establecimientos.txt");
    for (int i = 0; i < s.totalEstablecimientos; i++) escribirLinea(archivo, s.establecimientos[i]);
    archivo.close(); archivo.open("items.txt");
    for (int i = 0; i < s.totalEstablecimientos; i++)
        for (int j = 0; j < s.establecimientos[i].getCantidadItems(); j++)
            archivo << (i + 1) << "|" << s.establecimientos[i].getItem(j).nombre << "|" << s.establecimientos[i].getItem(j).precio << "\n";
    archivo.close(); archivo.open("mensajeros.txt");
    for (int i = 0; i < s.totalMensajeros; i++) escribirLinea(archivo, s.mensajeros[i]);
    archivo.close(); archivo.open("ordenes.txt");
    for (int i = 0; i < s.totalOrdenes; i++) {
        const Orden& o = s.ordenes[i];
        archivo << o.getIdOrden() << "|" << o.getIdCliente() << "|" << o.getIdEstablecimiento() << "|" << o.getIdMensajero() << "|" << o.getEstado()
                << "|" << o.getSubtotal() << "|" << o.getTarifaEnvio() << "|" << o.getDistanciaKm() << "\n";
    }
    archivo.close(); archivo.open("lineas.txt");
    for (int i = 0; i < s.totalOrdenes; i++)
        for (int j = 0; j < s.ordenes[i].getCantidadLineas(); j++)
            archivo << s.ordenes[i].getIdOrden() << "|" << s.ordenes[i].getPosicionItem(j) << "|" << s.ordenes[i].getCantidadPedida(j) << "\n";
    archivo.close(); archivo.open("historial.txt");
    for (int i = 0; i < s.totalOrdenes; i++)
        for (int j = 0; j < s.ordenes[i].getCantidadHistorial(); j++)
            archivo << s.ordenes[i].getIdOrden() << "|" << s.ordenes[i].getHistorial(j) << "\n";
}

void Almacen::cargar(Servicio& s) {
    ifstream archivo; string linea, partes[8];
    archivo.open("clientes.txt");
    while (getline(archivo, linea) && s.totalClientes < MAX_CLIENTES)
        if (separarPorBarra(linea, partes, 8) >= 4) {
            s.clientes[s.totalClientes].definir(textoAEntero(partes[0]), partes[1], Ubicacion(textoADecimal(partes[2]), textoADecimal(partes[3]))); s.totalClientes++;
        }
    archivo.close(); archivo.clear(); archivo.open("establecimientos.txt");
    while (getline(archivo, linea) && s.totalEstablecimientos < MAX_ESTABLECIMIENTOS)
        if (separarPorBarra(linea, partes, 8) >= 4) {
            s.establecimientos[s.totalEstablecimientos].definir(textoAEntero(partes[0]), partes[1], Ubicacion(textoADecimal(partes[2]), textoADecimal(partes[3]))); s.totalEstablecimientos++;
        }
    archivo.close(); archivo.clear(); archivo.open("items.txt");
    while (getline(archivo, linea))
        if (separarPorBarra(linea, partes, 8) >= 3) {
            int idEstablecimiento = textoAEntero(partes[0]);
            if (s.existeEstablecimiento(idEstablecimiento)) s.establecimientos[idEstablecimiento - 1].agregarItem(partes[1], textoADecimal(partes[2]));
        }
    archivo.close(); archivo.clear(); archivo.open("mensajeros.txt");
    while (getline(archivo, linea) && s.totalMensajeros < MAX_MENSAJEROS)
        if (separarPorBarra(linea, partes, 8) >= 6) {
            Mensajero& m = s.mensajeros[s.totalMensajeros];
            m.definir(textoAEntero(partes[0]), partes[1], Ubicacion(textoADecimal(partes[2]), textoADecimal(partes[3])));
            m.setVehiculo(partes[4]); m.setOcupado(textoAEntero(partes[5]) != 0); s.totalMensajeros++;
        }
    archivo.close(); archivo.clear(); archivo.open("ordenes.txt");
    while (getline(archivo, linea) && s.totalOrdenes < MAX_ORDENES)
        if (separarPorBarra(linea, partes, 8) >= 8) {
            s.ordenes[s.totalOrdenes].restaurar(textoAEntero(partes[0]), textoAEntero(partes[1]), textoAEntero(partes[2]), textoAEntero(partes[3]),
                                                 textoAEntero(partes[4]), textoADecimal(partes[5]), textoADecimal(partes[6]), textoADecimal(partes[7]));
            s.totalOrdenes++;
        }
    archivo.close(); archivo.clear(); archivo.open("lineas.txt");
    while (getline(archivo, linea))
        if (separarPorBarra(linea, partes, 8) >= 3) {
            int idOrden = textoAEntero(partes[0]);
            if (s.existeOrden(idOrden)) s.ordenes[idOrden - 1].restaurarLinea(textoAEntero(partes[1]), textoAEntero(partes[2]));
        }
    archivo.close(); archivo.clear(); archivo.open("historial.txt");
    while (getline(archivo, linea))
        if (separarPorBarra(linea, partes, 8) >= 2) {
            int idOrden = textoAEntero(partes[0]);
            if (s.existeOrden(idOrden)) s.ordenes[idOrden - 1].agregarAlHistorial(partes[1]);
        }
}

class Consola {
    Servicio servicio;
    bool entradaCerrada;

    string leerTexto(const string& mensaje) {
        string texto; cout << mensaje;
        if (!getline(cin, texto)) { entradaCerrada = true; texto = ""; }
        return texto;
    }
    int leerEntero(const string& mensaje) { string texto = leerTexto(mensaje); return esNumeroValido(texto) ? textoAEntero(texto) : -1; }
    double leerDecimal(const string& mensaje) {
        string texto = leerTexto(mensaje);
        while (!entradaCerrada && !esNumeroValido(texto)) texto = leerTexto("  Valor invalido, repita: ");
        return entradaCerrada ? 0 : textoADecimal(texto);
    }
    Ubicacion leerUbicacion() {
        double latitud = leerDecimal("  Latitud: "); double longitud = leerDecimal("  Longitud: ");
        return Ubicacion(latitud, longitud);
    }

    void imprimirEntidad(const Entidad& entidad) {
        cout << "  " << entidad.getId() << ") " << entidad.getNombre() << "\n";
    }
    void listarClientes() {
        for (int i = 1; i <= servicio.cantidadClientes(); i++) imprimirEntidad(servicio.cliente(i));
    }
    void listarEstablecimientos() {
        for (int i = 1; i <= servicio.cantidadEstablecimientos(); i++) imprimirEntidad(servicio.establecimiento(i));
    }

    void mostrarMenuPrincipal() {
        const int ANCHO = 25;
        const string cabeceras[3] = {"REGISTROS", "MENUS", "ORDENES"};
        const string celdas[5][3] = {
            {"1. Reg. cliente",          "4. Agregar item", "6. Crear orden"},
            {"2. Reg. establecimiento",  "5. Ver menu",     "7. Asignar mensajero"},
            {"3. Reg. mensajero",        "",                "8. Avanzar estado"},
            {"",                         "",                "9. Trazado de orden"},
            {"",                         "",                "10. Ver boleta"}
        };
        string borde = "+" + string(3 * ANCHO + 2, '-') + "+";
        string division = "+" + string(ANCHO, '-') + "+" + string(ANCHO, '-') + "+" + string(ANCHO, '-') + "+";
        cout << "\n" << borde << "\n|" << centrar("D E L I V E R Y", 3 * ANCHO + 2) << "|\n" << division << "\n|";
        for (int c = 0; c < 3; c++) cout << centrar(cabeceras[c], ANCHO) << "|";
        cout << "\n" << division << "\n";
        for (int f = 0; f < 5; f++) {
            cout << "|";
            for (int c = 0; c < 3; c++) cout << " " << rellenar(celdas[f][c], ANCHO - 2) << " |";
            cout << "\n";
        }
        cout << division << "\n|" << centrar("0. Salir", 3 * ANCHO + 2) << "|\n" << borde << "\n";
    }
    void mostrarMenu(int idEstablecimiento) {
        if (!servicio.existeEstablecimiento(idEstablecimiento)) { cout << "Establecimiento inexistente.\n"; return; }
        const Establecimiento& local = servicio.establecimiento(idEstablecimiento);
        cout << "Menu de " << local.getNombre() << ":\n";
        if (local.getCantidadItems() == 0) cout << "  (vacio)\n";
        for (int i = 0; i < local.getCantidadItems(); i++)
            cout << "  " << (i + 1) << ") " << local.getItem(i).nombre << " - $" << formatoDosDecimales(local.getItem(i).precio) << "\n";
    }
    void mostrarTrazado(int idOrden) {
        if (!servicio.existeOrden(idOrden)) { cout << "Orden inexistente.\n"; return; }
        const Orden& orden = servicio.orden(idOrden);
        const Establecimiento& local = servicio.establecimiento(orden.getIdEstablecimiento());
        const Cliente& cliente = servicio.cliente(orden.getIdCliente());
        const Entidad* participantes[3] = {&local, &cliente, nullptr};
        if (orden.tieneMensajero()) participantes[2] = &servicio.mensajero(orden.getIdMensajero());

        cout << "\n=== Orden " << orden.getIdOrden() << " | Estado: " << NOMBRES_ESTADO[orden.getEstado()] << " ===\n";
        for (int i = 0; i < 3; i++) {
            if (participantes[i] != nullptr) cout << participantes[i]->descripcion() << "\n";
            else cout << "Mensajero: (sin asignar)\n";
        }
        for (int i = 0; i < orden.getCantidadLineas(); i++) {
            const ItemMenu& item = local.getItem(orden.getPosicionItem(i));
            cout << "  " << orden.getCantidadPedida(i) << " x " << item.nombre << " ($" << formatoDosDecimales(item.precio) << ")\n";
        }
        cout << "Distancia: " << formatoDosDecimales(orden.getDistanciaKm()) << " km | Subtotal: $" << formatoDosDecimales(orden.getSubtotal())
             << " | Envio: $" << formatoDosDecimales(orden.getTarifaEnvio()) << " | TOTAL: $" << formatoDosDecimales(orden.getTotal()) << "\nHistorial:\n";
        for (int i = 0; i < orden.getCantidadHistorial(); i++) cout << "  " << (i + 1) << ". " << orden.getHistorial(i) << "\n";
    }
    void mostrarBoleta(int idOrden) {
        if (!servicio.existeOrden(idOrden)) { cout << "Orden inexistente.\n"; return; }
        const Orden& orden = servicio.orden(idOrden);
        const Establecimiento& local = servicio.establecimiento(orden.getIdEstablecimiento());
        const Cliente& cliente = servicio.cliente(orden.getIdCliente());
        const int ANCHO = 56;
        string doble(ANCHO, '='), simple(ANCHO, '-');

        cout << "\n" << doble << "\n" << centrar("BOLETA DE VENTA", ANCHO) << "\n"
             << centrar("Orden No. " + to_string(orden.getIdOrden()), ANCHO) << "\n" << doble << "\n";

        cout << "CLIENTE\n"
             << "  Nombre    : " << cliente.getNombre() << "\n"
             << "  Codigo    : " << cliente.getId() << "\n"
             << "  Ubicacion : " << cliente.getUbicacion().comoTexto() << "\n";

        cout << "ESTABLECIMIENTO\n"
             << "  Nombre    : " << local.getNombre() << "\n"
             << "  Ubicacion : " << local.getUbicacion().comoTexto() << "\n";

        cout << "MENSAJERO\n";
        if (orden.tieneMensajero()) {
            const Mensajero& m = servicio.mensajero(orden.getIdMensajero());
            cout << "  Nombre    : " << m.getNombre() << "\n"
                 << "  Vehiculo  : " << m.getVehiculo() << "\n";
        } else cout << "  (sin asignar)\n";

        cout << "ESTADO : " << NOMBRES_ESTADO[orden.getEstado()] << "\n" << simple << "\n";
        cout << rellenar("Cant", 6) << rellenar("Descripcion", 26) << rellenar("P.Unit", 11) << "Importe\n" << simple << "\n";
        for (int i = 0; i < orden.getCantidadLineas(); i++) {
            const ItemMenu& item = local.getItem(orden.getPosicionItem(i));
            int cantidad = orden.getCantidadPedida(i);
            cout << rellenar(to_string(cantidad), 6) << rellenar(item.nombre, 26)
                 << rellenar("$" + formatoDosDecimales(item.precio), 11)
                 << "$" << formatoDosDecimales(item.precio * cantidad) << "\n";
        }
        cout << simple << "\n"
             << rellenar("Distancia de envio", 43) << formatoDosDecimales(orden.getDistanciaKm()) << " km\n"
             << rellenar("Subtotal", 43) << "$" << formatoDosDecimales(orden.getSubtotal()) << "\n"
             << rellenar("Tarifa de envio", 43) << "$" << formatoDosDecimales(orden.getTarifaEnvio()) << "\n"
             << rellenar("TOTAL A PAGAR", 43) << "$" << formatoDosDecimales(orden.getTotal()) << "\n" << simple << "\n";

        cout << "HISTORIAL\n";
        for (int i = 0; i < orden.getCantidadHistorial(); i++)
            cout << "  " << (i + 1) << ". " << orden.getHistorial(i) << "\n";
        cout << doble << "\n" << centrar("Gracias por su compra", ANCHO) << "\n" << doble << "\n";
    }

    void registrarCliente() {
        string nombre = leerTexto("Nombre: "); Ubicacion ubicacion = leerUbicacion();
        if (entradaCerrada) return;
        cout << "Cliente " << servicio.registrarCliente(nombre, ubicacion) << " registrado.\n";
    }
    void registrarEstablecimiento() {
        string nombre = leerTexto("Nombre: "); Ubicacion ubicacion = leerUbicacion();
        if (entradaCerrada) return;
        cout << "Establecimiento " << servicio.registrarEstablecimiento(nombre, ubicacion) << " registrado.\n";
    }
    void registrarMensajero() {
        string nombre = leerTexto("Nombre: "), vehiculo = leerTexto("Vehiculo (moto/bici/auto): ");
        Ubicacion ubicacion = leerUbicacion();
        if (entradaCerrada) return;
        cout << "Mensajero " << servicio.registrarMensajero(nombre, vehiculo, ubicacion) << " registrado.\n";
    }
    void agregarItem() {
        listarEstablecimientos();
        int idEstablecimiento = leerEntero("Establecimiento: "); string nombreItem = leerTexto("Item: ");
        double precio = leerDecimal("Precio: ");
        if (entradaCerrada) return;
        cout << (servicio.agregarItemAlMenu(idEstablecimiento, nombreItem, precio) ? "Item agregado.\n" : "No se pudo agregar (id invalido, precio <= 0 o menu lleno).\n");
    }
    void verMenu() {
        listarEstablecimientos();
        mostrarMenu(leerEntero("Establecimiento: "));
    }
    void crearNuevaOrden() {
        if (servicio.cantidadClientes() == 0 || servicio.cantidadEstablecimientos() == 0) { cout << "Registre al menos un cliente y un establecimiento.\n"; return; }
        cout << "Clientes:\n"; listarClientes();
        int idCliente = leerEntero("Cliente: ");
        cout << "Establecimientos:\n"; listarEstablecimientos();
        int idEstablecimiento = leerEntero("Establecimiento: ");
        if (!servicio.existeEstablecimiento(idEstablecimiento)) { cout << "Establecimiento invalido.\n"; return; }
        mostrarMenu(idEstablecimiento);
        int itemsElegidos[MAX_LINEAS_ORDEN], cantidades[MAX_LINEAS_ORDEN], cantidadElegida = 0;
        while (cantidadElegida < MAX_LINEAS_ORDEN && !entradaCerrada) {
            int item = leerEntero("Item (0 = terminar): ");
            if (item == 0 || entradaCerrada) break;
            if (item < 1 || item > servicio.establecimiento(idEstablecimiento).getCantidadItems()) { cout << "  Item invalido.\n"; continue; }
            int cantidad = leerEntero("Cantidad: ");
            if (cantidad < 1) { cout << "  Cantidad invalida.\n"; continue; }
            itemsElegidos[cantidadElegida] = item; cantidades[cantidadElegida] = cantidad; cantidadElegida++;
        }
        int idOrden = servicio.crearOrden(idCliente, idEstablecimiento, itemsElegidos, cantidades, cantidadElegida);
        if (idOrden != 0) cout << "Orden " << idOrden << " creada.\n";
        else cout << "No se pudo crear la orden (datos invalidos o carrito vacio).\n";
    }
public:
    Consola() : entradaCerrada(false) {}

    void ejecutar() {
        int opcion = -1;
        while (opcion != 0 && !entradaCerrada) {
            mostrarMenuPrincipal();
            opcion = leerEntero("Opcion: ");
            if (entradaCerrada) break;
            if (opcion == 1) registrarCliente();
            else if (opcion == 2) registrarEstablecimiento();
            else if (opcion == 3) registrarMensajero();
            else if (opcion == 4) agregarItem();
            else if (opcion == 5) verMenu();
            else if (opcion == 6) crearNuevaOrden();
            else if (opcion == 7) cout << servicio.asignarMensajero(leerEntero("Id de orden: ")) << "\n";
            else if (opcion == 8) cout << servicio.avanzarEstado(leerEntero("Id de orden: ")) << "\n";
            else if (opcion == 9) mostrarTrazado(leerEntero("Id de orden: "));
            else if (opcion == 10) mostrarBoleta(leerEntero("Id de orden: "));
            else if (opcion != 0) cout << "Opcion invalida.\n";
        }
        if (entradaCerrada) cout << "\n";
    }
};

int main() {
    Consola consola;
    consola.ejecutar();
    return 0;
}
