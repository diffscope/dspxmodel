#ifndef DSPXMODEL_PARAMETERMAP_H
#define DSPXMODEL_PARAMETERMAP_H

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include <dspxmodelORM/DSPXModelORMGlobal.h>

namespace dspx {

    class Parameter;
    class SingingClip;

    class ParameterMapPrivate;

    /**
     * @brief Parameter map.
     */
    class DSPXMODEL_ORM_EXPORT ParameterMap : public QObject {
        Q_OBJECT
        Q_PROPERTY(int size READ size NOTIFY sizeChanged)
        Q_PROPERTY(QStringList keys READ keys NOTIFY keysChanged)
        Q_PROPERTY(QList<Parameter *> items READ items NOTIFY itemsChanged)
        Q_PROPERTY(SingingClip *singingClip READ singingClip CONSTANT)
    public:
        ~ParameterMap() override;

        /**
         * @brief Gets size.
         * @post size() >= 0.
         */
        int size() const;
        /**
         * @brief Gets keys.
         */
        QStringList keys() const;
        /**
         * @brief Gets items.
         */
        QList<Parameter *> items() const;

        /**
         * @brief Gets whether key is contained.
         */
        Q_INVOKABLE bool containsKey(const QString &key) const;
        /**
         * @brief Gets whether item is contained.
         */
        Q_INVOKABLE bool containsItem(Parameter *item) const;
        /**
         * @brief Gets item.
         */
        Q_INVOKABLE Parameter *item(const QString &key) const;
        /**
         * @brief Inserts item.
         * @pre item is not null.
         * @post If successful, item is contained in this map.
         * @returns true if successful, false if key is already contained in this map, or item is already contained in
         * this map or another map.
         */
        Q_INVOKABLE bool insertItem(const QString &key, Parameter *item);
        /**
         * @brief Removes item.
         * @post If successful, key is not contained in this map.
         * @returns true if successful, false if key is not contained in this map.
         */
        Q_INVOKABLE bool removeItem(const QString &key);
        /**
         * @brief Moves item.
         * @pre map is not null.
         * @post If successful, item is contained in map.
         * @returns true if successful, false if key is not contained in this map, or newKey is already contained in the
         * target map, or item is already contained in another map.
         */
        Q_INVOKABLE bool moveItem(const QString &key, ParameterMap *map, const QString &newKey);

        /**
         * @brief Gets singing clip.
         * @post singingClip() != nullptr.
         */
        SingingClip *singingClip() const;

    signals:
        void sizeChanged(int size);
        void keysChanged();
        void itemsChanged();
        void itemAboutToInsert(const QString &key, Parameter *item, ParameterMap *mapFromWhichMoved = nullptr);
        void itemInserted(const QString &key, Parameter *item, ParameterMap *mapFromWhichMoved = nullptr);
        void itemAboutToRemove(const QString &key, Parameter *item, ParameterMap *mapToWhichMoved = nullptr);
        void itemRemoved(const QString &key, Parameter *item, ParameterMap *mapToWhichMoved = nullptr);

    private:
        explicit ParameterMap(SingingClip *singingClip);

        QScopedPointer<ParameterMapPrivate> d_ptr;
    };

}

#endif // DSPXMODEL_PARAMETERMAP_H
