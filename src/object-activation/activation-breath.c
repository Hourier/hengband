#include "object-activation/activation-breath.h"
#include "object-enchant/dragon-breaths-table.h"
#include "object/object-flags.h"
#include "spell-kind/spells-launcher.h"
#include "spell-realm/spells-hex.h"
#include "system/object-type-definition.h"
#include "target/target-getter.h"
#include "util/bit-flags-calculator.h"
#include "view/display-messages.h"

/*!
 * @brief �����ɂ��u���X�̑������A�C�e���̑ϐ�����I�����A���s����������B/ Dragon breath activation
 * @details �ΏۂƂȂ�ϐ��� dragonbreath_info �e�[�u�����Q�Ƃ̂��ƁB
 * @param user_ptr �v���[���[�ւ̎Q�ƃ|�C���^
 * @param o_ptr �Ώۂ̃I�u�W�F�N�g�\���̃|�C���^
 * @return �������s�̐����Ԃ��B
 */
bool activate_dragon_breath(player_type *user_ptr, object_type *o_ptr)
{
    DIRECTION dir;
    if (!get_aim_dir(user_ptr, &dir))
        return FALSE;

    BIT_FLAGS resistance_flags[TR_FLAG_SIZE];
    object_flags(user_ptr, o_ptr, resistance_flags);

    int type[20];
    int n = 0;
    concptr name[20];
    for (int i = 0; dragonbreath_info[i].flag != 0; i++) {
        if (have_flag(resistance_flags, dragonbreath_info[i].flag)) {
            type[n] = dragonbreath_info[i].type;
            name[n] = dragonbreath_info[i].name;
            n++;
        }
    }

    if (n == 0)
        return FALSE;

    if (music_singing_any(user_ptr))
        stop_singing(user_ptr);

    if (hex_spelling_any(user_ptr))
        stop_hex_spell_all(user_ptr);

    int t = randint0(n);
    msg_format(_("���Ȃ���%s�̃u���X��f�����B", "You breathe %s."), name[t]);
    fire_breath(user_ptr, type[t], dir, 250, 4);
    return TRUE;
}