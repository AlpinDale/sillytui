#include <iostream>

#define SAFETENSORS_CPP_IMPLEMENTATION
#include "inference/model_loader/safetensors.hh"

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <model.safetensors>\n";
    return 1;
  }

  std::string filename = argv[1];

  safetensors::safetensors_t st;
  std::string warn, err;

  std::cout << "Loading model: " << filename << "\n";

  bool ret = safetensors::mmap_from_file(filename, &st, &warn, &err);

  if (warn.size()) {
    std::cout << "WARN: " << warn << "\n";
  }

  if (!ret) {
    std::cerr << "Failed to load: " << filename << "\n";
    std::cerr << "  ERR: " << err << "\n";
    return 1;
  }

  if (!safetensors::validate_data_offsets(st, err)) {
    std::cerr << "Invalid data_offsets: " << err << "\n";
    return 1;
  }

  std::cout << "Model loaded successfully!\n";
  std::cout << "\nModel Information:\n";
  std::cout << "  Tensors: " << st.tensors.size() << "\n";
  std::cout << "  Header size: " << st.header_size << " bytes\n";

  if (st.mmaped) {
    std::cout << "  Data size: " << st.databuffer_size << " bytes ("
              << (st.databuffer_size / (1024.0 * 1024.0)) << " MB)\n";
  }

  size_t total_params = 0;
  for (size_t i = 0; i < st.tensors.size(); i++) {
    safetensors::tensor_t tensor;
    st.tensors.at(i, &tensor);
    total_params += safetensors::get_shape_size(tensor);
  }

  std::cout << "  Parameters: " << total_params << " ("
            << (total_params / 1000000.0) << "M)\n";

  std::cout << "\nFirst 5 tensors:\n";
  for (size_t i = 0; i < std::min(st.tensors.size(), size_t(5)); i++) {
    std::string key = st.tensors.keys()[i];
    safetensors::tensor_t tensor;
    st.tensors.at(i, &tensor);

    std::cout << "  " << key << ": " << safetensors::get_dtype_str(tensor.dtype)
              << " [";
    for (size_t j = 0; j < tensor.shape.size(); j++) {
      if (j > 0)
        std::cout << ", ";
      std::cout << tensor.shape[j];
    }
    std::cout << "]\n";
  }

  return 0;
}
