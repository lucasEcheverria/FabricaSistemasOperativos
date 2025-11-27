# FabricaSistemasOperativos

20-Noviembre-2025:
Ya está implementado el hilo de ensamblaje, este simula el ensamblaje de productosm y una vez los tiene ensamblados envía una señal al semáforlo del hilo
de pintado, para que este pueda pintar las piezas ya ensambladas, admeás contamos con una variable protegida para saber cuantos productos están ensamblados.
27-Noviembre-2025
Ya está implementado el hilo de pintado, este simula el pintado de productos, que se inicia una vez se desbloquea el semáforo de ensamblaje, y una vez los tiene pintados envía una señal al semáforlo del hilo, para que el siguiente proceso pueda embalarlos. Además contamos con una variable protegida para saber cuantos productos están pintados.
