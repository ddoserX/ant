import os
import subprocess
import sys

def run_clang_tidy_on_directory(directory: str, clang_tidy_args: list):
    """
    Запускает clang-tidy для всех C++ файлов в указанной директории и её поддиректориях.
    Ошибки и предупреждения сохраняются в отдельные файлы в директории 'clang-tidy-errors'.

    Args:
        directory (str): Путь к директории для сканирования.
        clang_tidy_args (list): Список аргументов, которые нужно передать clang-tidy
                                после двойного тире (--). Например:
                                ['-Ivcpkg_installed/x64-linux/include', '-Isrc', '-std=c++23']
    """
    supported_extensions = ('.cpp', '.cxx', '.hpp', '.hxx')
    # Базовая команда clang-tidy, включая путь к файлу конфигурации.
    # Обратите внимание, что аргументы -- -I... -std=... передаются как separate_args
    # и будут добавлены после `--`
    base_clang_tidy_cmd = ['clang-tidy', '--config-file=.clang-tidy']

    # Директория для сохранения логов ошибок
    error_log_dir = os.path.join(directory, 'clang-tidy-errors')
    os.makedirs(error_log_dir, exist_ok=True) # Создаем директорию, если её нет

    print(f"Сканирование директории: {directory}")
    print(f"Логи ошибок будут сохранены в: {error_log_dir}")
    print("-" * 30)

    found_files = 0
    analyzed_files = 0
    errors_count = 0

    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(supported_extensions):
                found_files += 1
                filepath = os.path.join(root, file)
                # Получаем относительный путь для создания имени файла лога
                relative_filepath = os.path.relpath(filepath, directory)
                # Заменяем разделители пути на '_' и добавляем расширение .log
                log_filename = relative_filepath.replace(os.sep, '_') + '.log'
                log_filepath = os.path.join(error_log_dir, log_filename)

                print(f"Анализ файла: {filepath} -> Лог: {log_filepath}")

                # Формируем полную команду clang-tidy для текущего файла
                # Аргументы clang_tidy_args идут после --
                command = base_clang_tidy_cmd + [filepath, '--'] + clang_tidy_args

                try:
                    # Открываем файл для записи вывода clang-tidy
                    with open(log_filepath, 'w', encoding='utf-8') as log_file:
                        # Запускаем clang-tidy, перенаправляя stdout и stderr в файл
                        result = subprocess.run(
                            command,
                            stdout=log_file,
                            stderr=subprocess.STDOUT, # Объединяем stderr с stdout
                            text=True,
                            check=False
                        )

                        # Если код возврата ненулевой, это означает ошибки/предупреждения
                        if result.returncode != 0:
                            errors_count += 1
                            print(f"  [Ошибки/Предупреждения обнаружены, см. {log_filepath}]")
                        else:
                            print(f"  [Анализ завершен, без ошибок/предупреждений]")

                    analyzed_files += 1

                except FileNotFoundError:
                    print(f"Ошибка: clang-tidy не найден. Убедитесь, что он установлен и доступен в PATH.", file=sys.stderr)
                    return
                except Exception as e:
                    print(f"Произошла ошибка при запуске clang-tidy для {filepath}: {e}", file=sys.stderr)
                    errors_count += 1
    print("-" * 30)
    print(f"Сканирование завершено.")
    print(f"Найдено файлов C++: {found_files}")
    print(f"Проанализировано clang-tidy: {analyzed_files}")
    if errors_count > 0:
        print(f"Обнаружено ошибок в {errors_count} файлах. Подробности в директории '{error_log_dir}'.", file=sys.stderr)
    else:
        print("Ошибок clang-tidy не обнаружено.")


if __name__ == "__main__":
    target_directory = './src'

    # Аргументы, которые будут переданы clang-tidy после двойного тире (--)
    # Это соответствие вашему примеру вызова: -Ivcpkg_installed/x64-linux/include -Isrc -std=c++23
    # Убедитесь, что пути включения корректны для вашей системы.
    clang_tidy_compiler_args = [
        '-Ivcpkg_installed/x64-linux/include',
        '-Isrc',
        '-std=c++23' # Или '-std=c++20', в зависимости от вашей версии Clang и требований.
    ]

    run_clang_tidy_on_directory(target_directory, clang_tidy_compiler_args)
