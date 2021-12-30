#include <transform.h>


namespace view::dialogs
{

Transform::Transform()
{
	setupUi(this);
}

const Eigen::Affine2d &Transform::matrix() const
{
	return m_matrix;
}

void Transform::accept()
{
	QDialog::accept();

	m_matrix.translate(Eigen::Vector2d(offsetXSpinBox->value(), offsetYSpinBox->value()));
	m_matrix.rotate(angleSpinBox->value());
}

}
