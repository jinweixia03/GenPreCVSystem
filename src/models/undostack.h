#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include <QPixmap>
#include <QStack>
#include <QObject>

namespace GenPreCVSystem {
namespace Models {

/**
 * @brief 撤销/重做栈模型
 *
 * 管理图像编辑操作的撤销和重做状态
 */
class UndoStack : public QObject
{
    Q_OBJECT

public:
    static const int MAX_UNDO_STEPS = 50;  ///< 最大撤销步数

    explicit UndoStack(QObject *parent = nullptr)
        : QObject(parent)
    {}

    /**
     * @brief 保存当前状态到撤销栈
     * @param pixmap 当前图片状态
     */
    void push(const QPixmap &pixmap)
    {
        m_undoStack.push(pixmap);

        // 限制栈的大小，避免内存占用过大
        if (m_undoStack.size() > MAX_UNDO_STEPS) {
            m_undoStack.remove(0);  // 移除最旧的状态
        }

        // 清空重做栈（新操作会使之前的重做历史失效）
        m_redoStack.clear();

        emit stateChanged();
    }

    /**
     * @brief 撤销上一步操作
     * @param currentState 当前状态（将被保存到重做栈）
     * @return 撤销后的状态，如果栈为空返回空QPixmap
     */
    QPixmap undo(const QPixmap &currentState)
    {
        if (m_undoStack.isEmpty()) {
            return QPixmap();
        }

        // 将当前状态保存到重做栈
        m_redoStack.push(currentState);

        // 从撤销栈弹出上一个状态
        QPixmap previousState = m_undoStack.pop();

        emit stateChanged();

        return previousState;
    }

    /**
     * @brief 重做上一步撤销的操作
     * @param currentState 当前状态（将被保存到撤销栈）
     * @return 重做后的状态，如果栈为空返回空QPixmap
     */
    QPixmap redo(const QPixmap &currentState)
    {
        if (m_redoStack.isEmpty()) {
            return QPixmap();
        }

        // 将当前状态保存到撤销栈
        m_undoStack.push(currentState);

        // 从重做栈弹出状态
        QPixmap nextState = m_redoStack.pop();

        emit stateChanged();

        return nextState;
    }

    /**
     * @brief 清空所有状态
     */
    void clear()
    {
        m_undoStack.clear();
        m_redoStack.clear();
        emit stateChanged();
    }

    /**
     * @brief 是否可以撤销
     */
    bool canUndo() const { return !m_undoStack.isEmpty(); }

    /**
     * @brief 是否可以重做
     */
    bool canRedo() const { return !m_redoStack.isEmpty(); }

    /**
     * @brief 获取撤销步数
     */
    int undoCount() const { return m_undoStack.size(); }

    /**
     * @brief 获取重做步数
     */
    int redoCount() const { return m_redoStack.size(); }

signals:
    void stateChanged();

private:
    QStack<QPixmap> m_undoStack;  ///< 撤销栈
    QStack<QPixmap> m_redoStack;  ///< 重做栈
};

} // namespace Models
} // namespace GenPreCVSystem

#endif // UNDOSTACK_H
