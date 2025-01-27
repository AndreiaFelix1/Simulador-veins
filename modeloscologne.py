import pickle
import os

# Diretório onde os modelos estão localizados
diretorio_modelos = '/media/sf_modelos_colisao/cologne/modelo_global/colognepkl'

# Dicionário para armazenar os modelos
modelos = {}

# Nomes dos arquivos de modelos
nomes_modelos = [
    "modelo_dt_1step.pkl",
    "modelo_dt_2step.pkl",
    "modelo_dt_3step.pkl",
    "modelo_knn_1step.pkl",
    "modelo_knn_2step.pkl",
    "modelo_knn_3step.pkl",
    "modelo_rf_1step.pkl",
    "modelo_rf_2step.pkl",
    "modelo_rf_3step.pkl",
]

# Carregar os modelos no dicionário
for nome in nomes_modelos:
    caminho_completo = os.path.join(diretorio_modelos, nome)
    with open(caminho_completo, 'rb') as model_file:
        modelos[nome] = pickle.load(model_file)

def predict(nome_modelo, inputs):
    """
    Faz a previsão usando um dos modelos carregados.
    :param nome_modelo: Nome do modelo (deve corresponder aos arquivos .pkl).
    :param inputs: Dados de entrada para a previsão.
    :return: Resultado da previsão.
    """
    if nome_modelo not in modelos:
        raise ValueError(f"Modelo '{nome_modelo}' não encontrado. Verifique o nome.")
    
    modelo = modelos[nome_modelo]
    return modelo.predict([inputs])[0]

