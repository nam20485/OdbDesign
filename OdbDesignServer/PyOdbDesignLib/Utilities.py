from PyOdbDesignLib import PyOdbDesignLib


def load_file_archive(path):
    """
    This function loads a FileArchive object from disk and returns it.
    """
    try:
        file_archive = PyOdbDesignLib.FileArchive(path)
        return file_archive
    except Exception as e:
        print("Error: {}".format(e))
        return None
