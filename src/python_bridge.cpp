#include "python_bridge.h"
#include <stdio.h>
#include <string>

#if HAS_PYTHON
#include <Python.h>

static bool pythonInitialized = false;
static std::string scriptsPath;

bool PythonBridgeInit(const std::string& path) {
    scriptsPath = path;
    Py_Initialize();
    pythonInitialized = Py_IsInitialized() != 0;
    
    if (pythonInitialized) {
        // Add scripts directory to Python path
        std::string cmd = "import sys; sys.path.insert(0, '" + path + "')";
        PyRun_SimpleString(cmd.c_str());
        printf("[Python] Initialized successfully. Scripts path: %s\n", path.c_str());
    } else {
        printf("[Python] Failed to initialize!\n");
    }
    return pythonInitialized;
}

void PythonBridgeShutdown() {
    if (pythonInitialized) {
        Py_Finalize();
        pythonInitialized = false;
    }
}

static PyObject* ImportModule(const char* moduleName) {
    PyObject* pName = PyUnicode_DecodeFSDefault(moduleName);
    PyObject* pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if (!pModule) {
        PyErr_Print();
        printf("[Python] Failed to import module: %s\n", moduleName);
    }
    return pModule;
}

static double GetDictFloat(PyObject* dict, const char* key, double defaultVal) {
    PyObject* val = PyDict_GetItemString(dict, key);
    if (val && (PyFloat_Check(val) || PyLong_Check(val))) {
        return PyFloat_AsDouble(val);
    }
    return defaultVal;
}

static long GetDictInt(PyObject* dict, const char* key, long defaultVal) {
    PyObject* val = PyDict_GetItemString(dict, key);
    if (val && PyLong_Check(val)) {
        return PyLong_AsLong(val);
    }
    return defaultVal;
}

static const char* GetDictString(PyObject* dict, const char* key, const char* defaultVal) {
    PyObject* val = PyDict_GetItemString(dict, key);
    if (val && PyUnicode_Check(val)) {
        return PyUnicode_AsUTF8(val);
    }
    return defaultVal;
}

bool PythonBridgeLoadWeapons(WeaponStats outStats[3]) {
    if (!pythonInitialized) return false;
    
    PyObject* pModule = ImportModule("weapons");
    if (!pModule) return false;
    
    PyObject* pFunc = PyObject_GetAttrString(pModule, "get_weapons");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        printf("[Python] weapons.get_weapons() not found\n");
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
        return false;
    }
    
    PyObject* pResult = PyObject_CallObject(pFunc, NULL);
    if (!pResult || !PyList_Check(pResult)) {
        printf("[Python] weapons.get_weapons() didn't return a list\n");
        Py_XDECREF(pResult);
        Py_DECREF(pFunc);
        Py_DECREF(pModule);
        return false;
    }
    
    Py_ssize_t count = PyList_Size(pResult);
    if (count > 3) count = 3;
    
    for (Py_ssize_t i = 0; i < count; i++) {
        PyObject* weapon = PyList_GetItem(pResult, i);
        if (!PyDict_Check(weapon)) continue;
        
        outStats[i].name = GetDictString(weapon, "name", outStats[i].name.c_str());
        outStats[i].damage = (float)GetDictFloat(weapon, "damage", outStats[i].damage);
        outStats[i].fireRate = (float)GetDictFloat(weapon, "fire_rate", outStats[i].fireRate);
        outStats[i].magSize = (int)GetDictInt(weapon, "mag_size", outStats[i].magSize);
        outStats[i].reloadTime = (float)GetDictFloat(weapon, "reload_time", outStats[i].reloadTime);
        outStats[i].accuracy = (float)GetDictFloat(weapon, "accuracy", outStats[i].accuracy);
        outStats[i].recoilAmount = (float)GetDictFloat(weapon, "recoil", outStats[i].recoilAmount);
        outStats[i].range = (float)GetDictFloat(weapon, "range", outStats[i].range);
        outStats[i].pelletsPerShot = (int)GetDictInt(weapon, "pellets", outStats[i].pelletsPerShot);
        
        printf("[Python] Loaded weapon: %s (dmg=%.0f, rate=%.1f)\n", 
               outStats[i].name.c_str(), outStats[i].damage, outStats[i].fireRate);
    }
    
    Py_DECREF(pResult);
    Py_DECREF(pFunc);
    Py_DECREF(pModule);
    return true;
}

bool PythonBridgeLoadLevel(LevelData& level) {
    if (!pythonInitialized) return false;
    
    PyObject* pModule = ImportModule("level_arena");
    if (!pModule) return false;
    
    // Load enemy count
    PyObject* pEnemyCount = PyObject_GetAttrString(pModule, "enemy_count");
    if (pEnemyCount && PyLong_Check(pEnemyCount)) {
        level.enemyCount = (int)PyLong_AsLong(pEnemyCount);
        printf("[Python] Enemy count: %d\n", level.enemyCount);
    }
    Py_XDECREF(pEnemyCount);
    
    Py_DECREF(pModule);
    return true;
}

bool PythonBridgeLoadAIParams(float& detectionRange, float& attackRange, float& accuracy, float& reactionTime) {
    if (!pythonInitialized) return false;
    
    PyObject* pModule = ImportModule("ai_behavior");
    if (!pModule) return false;
    
    PyObject* pFunc = PyObject_GetAttrString(pModule, "get_ai_params");
    if (!pFunc || !PyCallable_Check(pFunc)) {
        Py_XDECREF(pFunc);
        Py_DECREF(pModule);
        return false;
    }
    
    PyObject* pResult = PyObject_CallObject(pFunc, NULL);
    if (pResult && PyDict_Check(pResult)) {
        detectionRange = (float)GetDictFloat(pResult, "detection_range", detectionRange);
        attackRange = (float)GetDictFloat(pResult, "attack_range", attackRange);
        accuracy = (float)GetDictFloat(pResult, "accuracy", accuracy);
        reactionTime = (float)GetDictFloat(pResult, "reaction_time", reactionTime);
        printf("[Python] AI params loaded (detect=%.0f, attack=%.0f)\n", detectionRange, attackRange);
    }
    
    Py_XDECREF(pResult);
    Py_DECREF(pFunc);
    Py_DECREF(pModule);
    return true;
}

void PythonBridgeOnKill(int killCount) {
    if (!pythonInitialized) return;
    
    PyObject* pModule = ImportModule("events");
    if (!pModule) return;
    
    PyObject* pFunc = PyObject_GetAttrString(pModule, "on_kill");
    if (pFunc && PyCallable_Check(pFunc)) {
        PyObject* pArgs = Py_BuildValue("(i)", killCount);
        PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
        Py_XDECREF(pResult);
        Py_XDECREF(pArgs);
    }
    
    Py_XDECREF(pFunc);
    Py_DECREF(pModule);
}

void PythonBridgeOnDamage(float damage, float healthRemaining) {
    if (!pythonInitialized) return;
    
    PyObject* pModule = ImportModule("events");
    if (!pModule) return;
    
    PyObject* pFunc = PyObject_GetAttrString(pModule, "on_damage");
    if (pFunc && PyCallable_Check(pFunc)) {
        PyObject* pArgs = Py_BuildValue("(ff)", damage, healthRemaining);
        PyObject* pResult = PyObject_CallObject(pFunc, pArgs);
        Py_XDECREF(pResult);
        Py_XDECREF(pArgs);
    }
    
    Py_XDECREF(pFunc);
    Py_DECREF(pModule);
}

#else // No Python support

bool PythonBridgeInit(const std::string& path) {
    printf("[Python] Built without Python support. Using default configs.\n");
    (void)path;
    return false;
}

void PythonBridgeShutdown() {}

bool PythonBridgeLoadWeapons(WeaponStats outStats[3]) {
    (void)outStats;
    return false;
}

bool PythonBridgeLoadLevel(LevelData& level) {
    (void)level;
    return false;
}

bool PythonBridgeLoadAIParams(float& detectionRange, float& attackRange, float& accuracy, float& reactionTime) {
    (void)detectionRange; (void)attackRange; (void)accuracy; (void)reactionTime;
    return false;
}

void PythonBridgeOnKill(int killCount) { (void)killCount; }
void PythonBridgeOnDamage(float damage, float healthRemaining) { (void)damage; (void)healthRemaining; }

#endif
