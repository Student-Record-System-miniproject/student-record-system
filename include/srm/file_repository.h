#ifndef SRM_FILE_REPOSITORY_H
#define SRM_FILE_REPOSITORY_H

#include "models.h"

#include <string>
#include <vector>

namespace srm {

class FileRepository {
public:
    explicit FileRepository(const std::string& dataDirectory);

    bool saveStudents(const std::vector<Student>& students);
    bool loadStudents(std::vector<Student>& students) const;

    bool saveUsers(const std::vector<User>& users);
    bool loadUsers(std::vector<User>& users) const;

private:
    std::string dataDirectory;

    std::string studentsFilePath() const;
    std::string usersFilePath() const;
};

} // namespace srm

#endif